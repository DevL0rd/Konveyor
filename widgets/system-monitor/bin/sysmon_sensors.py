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
