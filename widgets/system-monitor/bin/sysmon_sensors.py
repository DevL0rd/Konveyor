import glob
import os
import re
import time

SENSOR_RESCAN_SECONDS = 60
_sensors = {"when": None, "freq": [], "temp": [], "fan": None}


def _open_sensor(path):
    try:
        return os.open(path, os.O_RDONLY)
    except OSError:
        return None


def _close_sensors():
    for _, fd in _sensors["freq"]:
        os.close(fd)
    for _, fd in _sensors["temp"]:
        os.close(fd)
    if _sensors["fan"] is not None:
        os.close(_sensors["fan"])
    _sensors["freq"], _sensors["temp"], _sensors["fan"] = [], [], None


def _scan_sensors(now):
    _close_sensors()
    freq = []
    for path in glob.glob("/sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_cur_freq"):
        match = re.search(r"cpu(\d+)", path)
        fd = _open_sensor(path) if match else None
        if fd is not None:
            freq.append((int(match.group(1)), fd))
    temp = []
    fan = None
    for hw in glob.glob("/sys/class/hwmon/hwmon*"):
        try:
            name = open(os.path.join(hw, "name")).read().strip()
        except OSError:
            continue
        if name in ("coretemp", "k10temp", "zenpower"):
            for ti in glob.glob(os.path.join(hw, "temp*_input")):
                try:
                    label = open(ti[:-6] + "_label").read().strip().lower()
                except OSError:
                    label = ""
                kind = "package" if "package" in label else label if label in ("tdie", "tctl") else "core"
                fd = _open_sensor(ti)
                if fd is not None:
                    temp.append((kind, fd))
        if fan is None:
            for fi in glob.glob(os.path.join(hw, "fan*_input")):
                try:
                    if "cpu" in open(fi[:-6] + "_label").read().strip().lower():
                        fan = _open_sensor(fi)
                        break
                except OSError:
                    continue
    _sensors.update(freq=freq, temp=temp, fan=fan, when=now)


def _sensors_fresh():
    now = time.monotonic()
    if _sensors["when"] is None or now - _sensors["when"] > SENSOR_RESCAN_SECONDS:
        _scan_sensors(now)


def _read_number(fd):
    return int(os.pread(fd, 32, 0))


def cpu_freqs():
    """logical cpu index -> current MHz."""
    _sensors_fresh()
    f = {}
    for n, fd in _sensors["freq"]:
        try:
            f[n] = _read_number(fd) / 1000.0
        except (OSError, ValueError):
            _sensors["when"] = None
    return f


def cpu_temp():
    """Average of the per-core sensors (Intel 'Core N', AMD 'Tccd N'). CPUs without
    per-core sensors report their package/die sensor (Package id 0, Tdie, Tctl)."""
    _sensors_fresh()
    pkg = tdie = tctl = None
    cores = []
    for kind, fd in _sensors["temp"]:
        try:
            t = _read_number(fd) / 1000.0
        except (OSError, ValueError):
            _sensors["when"] = None
            continue
        if kind == "package":
            pkg = t
        elif kind == "tdie":
            tdie = t
        elif kind == "tctl":
            tctl = t
        else:
            cores.append(t)
    if cores:
        return round(sum(cores) / len(cores))
    best = pkg if pkg is not None else tdie if tdie is not None else tctl
    return round(best) if best is not None else 0


def cpu_fan():
    """RPM of a fan whose hwmon label contains 'cpu' (generic, any board/laptop).
    None when no such sensor exists."""
    _sensors_fresh()
    fd = _sensors["fan"]
    if fd is None:
        return None
    try:
        return _read_number(fd)
    except (OSError, ValueError):
        _sensors["when"] = None
        return None


def _read(path, cast=int):
    try:
        return cast(open(path).read().strip())
    except (OSError, ValueError):
        return None


def _hwdb_name(dev):
    """The device's name in the udev hardware database, e.g. "Navi 32 [Radeon
    RX 7700 XT / 7800 XT]" -> "Radeon RX 7700 XT / 7800 XT". None if udev
    doesn't know it."""
    try:
        slot = re.search(r"^PCI_SLOT_NAME=(.+)$", open(os.path.join(dev, "uevent")).read(), re.M).group(1)
        model = re.search(r"^E:ID_MODEL_FROM_DATABASE=(.+)$", open("/run/udev/data/+pci:" + slot).read(), re.M).group(1)
    except (OSError, AttributeError):
        return None
    brand = re.search(r"\[(.+)\]", model)
    return brand.group(1) if brand else model


_amd = {}


def _amd_card(cpu_model):
    """The amdgpu card with the most VRAM (a discrete card over the APU), its
    name, hwmon dir, and whether it's an APU. gpu_metrics format revision 1 is
    the discrete-GPU layout; APUs use 2 and up."""
    cards = [os.path.dirname(p) for p in glob.glob("/sys/class/drm/card[0-9]*/device/gpu_busy_percent")]
    if not cards:
        return None
    dev = max(cards, key=lambda d: _read(os.path.join(d, "mem_info_vram_total")) or 0)
    hwmon = (glob.glob(os.path.join(dev, "hwmon", "hwmon*")) or [None])[0]
    try:
        with open(os.path.join(dev, "gpu_metrics"), "rb") as f:
            apu = f.read(3)[2] >= 2
    except (OSError, IndexError):
        apu = False
    try:
        sclk = [int(ln.split()[1].lower().rstrip("mhz")) for ln in open(os.path.join(dev, "pp_dpm_sclk"))]
    except (OSError, ValueError, IndexError):
        sclk = [0]
    # The CPU model names an APU exactly ("... w/ Radeon 860M"); udev only
    # knows the die ("Radeon 840M / 860M Graphics").
    if " w/ " in cpu_model and apu:
        name = "AMD " + cpu_model.partition(" w/ ")[2]
    else:
        name = _read(os.path.join(dev, "product_name"), str) or "AMD " + (_hwdb_name(dev) or "GPU")
    return {"dev": dev, "name": name, "hwmon": hwmon, "apu": apu, "clock_max": max(sclk)}


def amd_gpu(cpu_model=""):
    """AMD GPU stats from amdgpu sysfs, in read_gpu()'s shape. None without an
    amdgpu card. On an APU the memory bar covers VRAM + GTT, since most of what
    the GPU uses is system RAM mapped through GTT."""
    if "card" not in _amd:
        _amd["card"] = _amd_card(cpu_model)
    card = _amd["card"]
    if card is None:
        return None
    dev, hw = card["dev"], card["hwmon"]
    # A runtime-suspended discrete GPU would wake up to answer; report it idle.
    awake = _read(os.path.join(dev, "power", "runtime_status"), str) != "suspended"

    def mem(kind):
        return ((_read(os.path.join(dev, "mem_info_%s_used" % kind)) or 0) if awake else 0,
                _read(os.path.join(dev, "mem_info_%s_total" % kind)) or 0)

    def sensor(name, scale=1):
        v = _read(os.path.join(hw, name)) if hw and awake else None
        return v / scale if v is not None else None

    used, total = mem("vram")
    if card["apu"]:
        gtt_used, gtt_total = mem("gtt")
        used, total = used + gtt_used, total + gtt_total
    power = sensor("power1_average", 1e6)
    pwm, pwm_max = sensor("pwm1"), sensor("pwm1_max")
    return {
        "name": card["name"],
        "util": (_read(os.path.join(dev, "gpu_busy_percent")) or 0) if awake else 0,
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
