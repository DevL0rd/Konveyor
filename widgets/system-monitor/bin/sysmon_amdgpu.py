import glob
import os
import re

from sysmon_sysfs import awake, hwdb_name, read

_amd = {"card": None}


def _find_card():
    """The amdgpu card with the most VRAM (a discrete card over the APU). The
    files read here are static and never wake a runtime-suspended card."""
    cards = [os.path.dirname(p) for p in sorted(glob.glob("/sys/class/drm/card[0-9]*/device/gpu_busy_percent"))]
    if not cards:
        return None
    dev = max(cards, key=lambda d: read(os.path.join(d, "mem_info_vram_total")) or 0)
    name = read(os.path.join(dev, "product_name"), str) or "AMD " + (hwdb_name(dev) or "GPU")
    return {"dev": dev, "name": name, "hwmon": None, "apu": False, "clock_max": 0, "probed": False}


def _probe(card, cpu_model):
    """gpu_metrics and pp_dpm_sclk wake a suspended card, so they are read only
    once it is awake. gpu_metrics format revision 1 is the discrete-GPU layout;
    APUs use 2 and up. The CPU model names an APU exactly ("... w/ Radeon
    860M"); udev only knows the die ("Radeon 840M / 860M Graphics")."""
    dev = card["dev"]
    try:
        with open(os.path.join(dev, "gpu_metrics"), "rb") as f:
            card["apu"] = f.read(3)[2] >= 2
    except (OSError, IndexError):
        card["apu"] = False
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
    reported idle without touching any file that would wake it."""
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
        "util": (read(os.path.join(dev, "gpu_busy_percent")) or 0) if powered else 0,
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
