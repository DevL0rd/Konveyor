import glob
import os
import time

from sysmon_amdgpu import _awake, _hwdb_name, _read

_intel = {"card": None}


def _find_card():
    """The first i915 GT. Its name and max clock are static and never wake a
    runtime-suspended GPU."""
    gts = sorted(glob.glob("/sys/class/drm/card[0-9]*/gt/gt0/rc6_residency_ms"))
    if not gts:
        return None
    gt = os.path.dirname(gts[0])
    dev = os.path.join(os.path.dirname(os.path.dirname(gt)), "device")
    return {"gt": gt, "dev": dev, "name": "Intel " + (_hwdb_name(dev) or "Graphics"),
            "clock_max": _read(os.path.join(gt, "rps_RP0_freq_mhz")) or 0, "prev": None}


def intel_gpu():
    """Intel GPU stats from i915 sysfs, in read_gpu()'s shape. None without an
    i915 card. Usage is the share of time the GT spent out of RC6, which reads
    about 10 points above the engines' own busy time at low load (the GT wakes
    for more than rendering) and matches it under real load. i915 exposes no
    memory, temperature or power for integrated GPUs. Reading RC6 residency
    wakes a runtime-suspended GPU, so a suspended one is reported idle."""
    card = _intel["card"]
    if card is None or not os.path.exists(os.path.join(card["gt"], "rc6_residency_ms")):
        card = _intel["card"] = _find_card()
    if card is None:
        return None
    util = clock = 0
    if _awake(card["dev"]):
        now, rc6 = time.monotonic(), _read(os.path.join(card["gt"], "rc6_residency_ms"))
        prev, card["prev"] = card["prev"], (now, rc6)
        if prev and None not in (prev[1], rc6) and now > prev[0]:
            util = round(min(100, max(0, 100 - (rc6 - prev[1]) / ((now - prev[0]) * 10))))
        clock = _read(os.path.join(card["gt"], "rps_act_freq_mhz")) or 0
    else:
        card["prev"] = None
    return {
        "name": card["name"], "util": util,
        "vram_used": 0, "vram_total": 0, "vram_pct": 0, "temp": 0, "power": 0,
        "clock_gr": clock, "clock_mem": 0, "fan": 0,
        "clock_max": card["clock_max"], "power_max": 0, "shared": True,
    }
