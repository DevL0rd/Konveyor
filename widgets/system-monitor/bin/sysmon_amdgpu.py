import glob
import os
import re
import time

from sysmon_sysfs import awake, hwdb_name, pci_slot, read

_amd = {"card": None}
_SMU10 = {0x15DD, 0x15D8}
_ENGINES = ("gfx", "compute")


def _find_card():
    """The amdgpu card with the most VRAM (a discrete card over the APU). The
    files read here are static and never wake a runtime-suspended card."""
    cards = [os.path.dirname(p) for p in sorted(glob.glob("/sys/class/drm/card[0-9]*/device/gpu_busy_percent"))]
    if not cards:
        return None
    dev = max(cards, key=lambda d: read(os.path.join(d, "mem_info_vram_total")) or 0)
    name = read(os.path.join(dev, "product_name"), str) or "AMD " + (hwdb_name(dev) or "GPU")
    smu10 = read(os.path.join(dev, "device"), lambda text: int(text, 16)) in _SMU10
    slot = pci_slot(dev) if smu10 else None
    if smu10 and slot is None:
        raise RuntimeError("PCI_SLOT_NAME is missing from %s, which the fdinfo load of a Raven or Picasso card needs" % os.path.join(dev, "uevent"))
    return {"dev": dev, "name": name, "hwmon": None, "apu": smu10, "clock_max": 0, "probed": False,
            "slot": slot, "engines": None}


def _engine_time(slot):
    """Busy nanoseconds per (DRM client, engine) on the card at PCI `slot`, from
    the fdinfo of every DRM file this user can see. A client is counted once
    however many file descriptors share it."""
    times = {}
    for pid in os.listdir("/proc"):
        if not pid.isdigit():
            continue
        fds = "/proc/%s/fd/" % pid
        try:
            names = os.listdir(fds)
        except OSError:
            continue
        for fd in names:
            try:
                if not os.readlink(fds + fd).startswith("/dev/dri/"):
                    continue
                with open("/proc/%s/fdinfo/%s" % (pid, fd)) as f:
                    info = {key: value.strip() for key, _, value in (line.partition(":") for line in f)}
            except OSError:
                continue
            if info.get("drm-pdev") != slot or "drm-client-id" not in info:
                continue
            for engine in _ENGINES:
                ns = info.get("drm-engine-" + engine, "").split()
                if ns and ns[0].isdigit():
                    times[info["drm-client-id"], engine] = int(ns[0])
    return times


def _fdinfo_load(card):
    """The busiest engine's share of the time since the last call, in percent;
    0 on the first call. A client that appeared since then counts in full."""
    now, times = time.monotonic(), _engine_time(card["slot"])
    prev, card["engines"] = card["engines"], (now, times)
    if prev is None or now <= prev[0]:
        return 0
    busy = {}
    for (client, engine), ns in times.items():
        busy[engine] = busy.get(engine, 0) + max(0, ns - prev[1].get((client, engine), 0))
    return round(min(100, max(busy.values(), default=0) / ((now - prev[0]) * 1e7)))


def _probe(card, cpu_model):
    """gpu_metrics and pp_dpm_sclk wake a suspended card, so they are read only
    once it is awake. gpu_metrics format revision 1 is the discrete-GPU layout;
    APUs use 2 and up. APUs without gpu_metrics at all are already known by
    PCI ID from _find_card. The CPU model names an APU exactly ("... w/ Radeon
    860M"); udev only knows the die ("Radeon 840M / 860M Graphics")."""
    dev = card["dev"]
    try:
        with open(os.path.join(dev, "gpu_metrics"), "rb") as f:
            card["apu"] = card["apu"] or f.read(3)[2] >= 2
    except (OSError, IndexError):
        pass
    sclk = read(os.path.join(dev, "pp_dpm_sclk"), lambda text: [int(m) for m in re.findall(r"(\d+)\s*mhz", text, re.I)])
    card["clock_max"] = max(sclk or [0])
    if " w/ " in cpu_model and card["apu"]:
        card["name"] = "AMD " + cpu_model.partition(" w/ ")[2]
    card["probed"] = True


def _hwmon(card):
    if not (card["hwmon"] and os.path.isdir(card["hwmon"])):
        card["hwmon"] = (sorted(glob.glob(os.path.join(card["dev"], "hwmon", "hwmon*"))) or [None])[0]
    return card["hwmon"]


def amd_gpu(cpu_model=""):
    """AMD GPU stats from amdgpu sysfs, in read_gpu()'s shape. None without an
    amdgpu card. On an APU the memory bar covers VRAM + GTT, since most of what
    the GPU uses is system RAM mapped through GTT. A runtime-suspended card is
    reported idle without touching any file that would wake it. On Raven and
    Picasso the load comes from fdinfo, as gpu_busy_percent is stuck at 100.
    Raises RuntimeError when such a card has no PCI_SLOT_NAME in its uevent."""
    card = _amd["card"]
    if card is None or not os.path.exists(os.path.join(card["dev"], "gpu_busy_percent")):
        card = _amd["card"] = _find_card()
    if card is None:
        return None
    dev = card["dev"]
    powered = awake(dev)
    if powered and not card["probed"]:
        _probe(card, cpu_model)
    hw = _hwmon(card)

    def mem(kind):
        return ((read(os.path.join(dev, "mem_info_%s_used" % kind)) or 0) if powered else 0,
                read(os.path.join(dev, "mem_info_%s_total" % kind)) or 0)

    def sensor(name, scale=1):
        v = read(os.path.join(hw, name)) if hw and powered else None
        return v / scale if v is not None else None

    used, total = mem("vram")
    if card["apu"]:
        gtt_used, gtt_total = mem("gtt")
        used, total = used + gtt_used, total + gtt_total
    power = sensor("power1_average", 1e6)
    pwm, pwm_max = sensor("pwm1"), sensor("pwm1_max")
    return {
        "name": card["name"],
        "util": ((_fdinfo_load(card) if card["slot"] else read(os.path.join(dev, "gpu_busy_percent")) or 0)
                 if powered else 0),
        "vram_used": used, "vram_total": total,
        "vram_pct": (used / total * 100) if total else 0,
        "temp": sensor("temp1_input", 1000) or 0,
        "power": round(power if power is not None else sensor("power1_input", 1e6) or 0, 1),
        "clock_gr": round(sensor("freq1_input", 1e6) or 0),
        "clock_mem": round(sensor("freq2_input", 1e6) or 0),
        "fan": round(pwm / pwm_max * 100) if pwm is not None and pwm_max else 0,
        "clock_max": card["clock_max"],
        "power_max": round(sensor("power1_cap", 1e6) or 0),
        "shared": card["apu"],
    }
