#!/usr/bin/env python3
import subprocess
import sys
import time
from pathlib import Path

import dbus

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from borderprobe import CYAN, TITLES, edge_pixels, painted, painted_titles, probe_config, screenshot, settle_to, solid
from checks import Checks, load_config
from kwinsession import for_window, run_script, wait_for

UPOWER = Path(__file__).resolve().parent.parent / "harness" / "upower.py"
SPIN = 'border-style "Spin" {\n    layer "dashes" { colors "#ff00ff"; speed 3; dash 20; gap 20; width 8; }\n}\n'
RING = 'width 8\n        active-style "Spin"'
EVENT = '        on "focus" "Mark" reach="all" duration=4'
SERVICE = "org.freedesktop.UPower"
PATH = "/org/freedesktop/UPower"
CONTROL = "org.konveyor.FakeUPower"
SERVICES = []




def load(animations=""):
    return load_config(probe_config(EVENT, solid("Mark", "#00ffff") + SPIN, ring=RING, animations=animations))


def ring_frames(title, count=3, pause=0.3):
    frames = []
    for _ in range(count):
        frames.append(edge_pixels(screenshot(), title))
        time.sleep(pause)
    return frames


def moving(title):
    frames = ring_frames(title)
    return any(frame != frames[0] for frame in frames[1:])


def expect_motion(checks, title, expected, message):
    checks.expect(wait_for(lambda: not painted(screenshot(), title, CYAN) and moving(title) == expected, 30, 0.2), message)


def expect_effect(checks, expected, message):
    settle_to("A")
    settle_to("B")
    if expected:
        checks.expect(wait_for(lambda: {"A", "C"} <= painted_titles(screenshot(), TITLES, CYAN), 15, 0.3), message)
    else:
        time.sleep(1.5)
        checks.expect(not painted_titles(screenshot(), TITLES, CYAN), message)


def control(method, value):
    proxy = dbus.SystemBus().get_object(SERVICE, PATH)
    getattr(dbus.Interface(proxy, CONTROL), method)(value)


def start_upower(mode):
    process = subprocess.Popen([sys.executable, str(UPOWER), mode])
    SERVICES.append(process)
    deadline = time.monotonic() + 20
    while time.monotonic() < deadline:
        if SERVICE in dbus.SystemBus().list_names():
            return process
        time.sleep(0.1)
    raise RuntimeError("the fake UPower never appeared")


def stop_upower(process):
    process.terminate()
    process.wait(timeout=20)
    SERVICES.remove(process)
    wait_for(lambda: SERVICE not in dbus.SystemBus().list_names(), 20)


def animated_without_a_power_service(checks):
    checks.expect(load(), "a ring style and a focus effect load")
    settle_to("B")
    expect_motion(checks, "B", True, "with no UPower on the bus the ring animates")
    expect_effect(checks, True, "and moving focus plays the effect on the other windows")


def battery_power_holds_everything_still(checks):
    upower = start_upower("ac")
    expect_motion(checks, "B", True, "on mains power the ring keeps animating")
    control("Set", True)
    expect_motion(checks, "B", False, "a power-state change to battery freezes the ring")
    expect_effect(checks, False, "and focus changes play no effect")
    control("Set", False)
    expect_motion(checks, "B", True, "back on mains the ring animates again")
    expect_effect(checks, True, "and the effect plays again")
    control("Invalidate", True)
    expect_motion(checks, "B", False, "an invalidated OnBattery is read again and freezes the ring")
    control("Unrelated", False)
    time.sleep(1.0)
    expect_motion(checks, "B", False, "a property change on another interface is ignored")
    control("OtherProperty", False)
    time.sleep(1.0)
    expect_motion(checks, "B", False, "another property of the UPower service is ignored")
    control("Set", True)
    stop_upower(upower)
    expect_motion(checks, "B", True, "UPower leaving the bus counts as mains power")
    again = start_upower("battery")
    expect_motion(checks, "B", False, "UPower returning on battery freezes the ring from its first answer")
    control("Set", False)
    expect_motion(checks, "B", True, "and mains power releases it")
    stop_upower(again)


def reduced_motion_holds_everything_still(checks):
    checks.expect(load("reduce-motion"), "a config with reduce-motion loads")
    expect_motion(checks, "B", False, "reduced motion freezes the ring")
    expect_effect(checks, False, "and plays no event effect")
    checks.expect(load(), "the config without it loads")
    expect_motion(checks, "B", True, "motion returns")
    expect_effect(checks, True, "and so do the effects")


def fullscreen_monitors_get_no_effect(checks):
    settle_to("A")
    run_script(for_window("A", "w.fullScreen = true;"))
    time.sleep(1.0)
    settle_to("B")
    settle_to("A")
    time.sleep(1.0)
    checks.expect(not painted_titles(screenshot(), TITLES, CYAN), "focus changes on a monitor showing a fullscreen window play nothing")
    run_script(for_window("A", "w.fullScreen = false;"))
    time.sleep(1.0)
    expect_effect(checks, True, "and the effect plays once the fullscreen window is gone")
    settle_to("A")
    run_script(for_window("C", "w.fullScreen = true;"))
    time.sleep(1.0)
    run_script(for_window("C", "w.fullScreen = false;"))


def main():
    try:
        Checks().run(animated_without_a_power_service, battery_power_holds_everything_still, reduced_motion_holds_everything_still,
                     fullscreen_monitors_get_no_effect)
    finally:
        for process in SERVICES:
            process.terminate()


if __name__ == "__main__":
    main()
