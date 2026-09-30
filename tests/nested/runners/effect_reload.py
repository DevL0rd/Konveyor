#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import CLIENTS, for_window, managed_now, reload_konveyor, run_script, wait_for, window_state

def reload_effect(checks, titles):
    checks.expect(reload_konveyor(titles), f"the reloaded effect manages the open windows ({sorted(managed_now())})")


def fullscreen(title):
    return (window_state(title) or "").startswith("true|")


def fullscreen_survives(checks):
    subprocess.Popen(["qml6", str(CLIENTS / "fullscreen-client.qml"), "--", "/nonexistent"])
    subprocess.Popen(["python3", str(CLIENTS / "x11-types.py"), "X11Full:NORMAL"])
    checks.expect(wait_for(lambda: "X11Full" in managed_now(), 60), "the X11 window opened")
    run_script(for_window("X11Full", "w.fullScreen = true;"))
    titles = ("SelfFullscreen", "X11Full")
    checks.expect(wait_for(lambda: all(fullscreen(title) for title in titles), 60), "the Wayland and X11 windows made themselves fullscreen")
    reload_effect(checks, titles)
    for title in titles:
        checks.expect(wait_for(lambda: fullscreen(title), 10) and not wait_for(lambda: not fullscreen(title), 3),
                      f"{title} is still fullscreen after the effect reloaded ({window_state(title)})")


def main():
    Checks().run(fullscreen_survives)


if __name__ == "__main__":
    main()
