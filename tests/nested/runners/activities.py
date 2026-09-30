#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import active_title, for_window, konveyor_windows, qdbus, run_script, wait_for

MANAGER = ("org.kde.ActivityManager", "/ActivityManager/Activities", "org.kde.ActivityManager.Activities")


def activities(method, *arguments):
    service, path, interface = MANAGER
    return qdbus(service, path, f"{interface}.{method}", *arguments)


def manager_running():
    return subprocess.run(["qdbus6", *MANAGER[:2]], capture_output=True).returncode == 0


def kwin_activity():
    return run_script('print("MARK|" + workspace.currentActivity);')[0]


def row():
    return {window["title"]: window["layout"]["pos_in_scrolling_layout"][0] for window in konveyor_windows()}


def switch_to(checks, activity):
    activities("SetCurrentActivity", activity)
    checks.expect(wait_for(lambda: kwin_activity() == activity), f"KWin switched to activity {activity}")


def only_current_activity_takes_room(checks):
    home = activities("CurrentActivity")
    other = activities("AddActivity", "Other")
    checks.expect(wait_for(lambda: other in run_script('print("MARK|" + workspace.activities.join(","));')[0]), "KWin knows the new activity")
    for title in ("A", "B", "C", "D"):
        run_script(for_window(title, f'w.activities = ["{home}"];'))
    checks.equal(row(), {"A": 1, "B": 2, "C": 3, "D": 4}, "the four windows start in four columns")
    run_script(for_window("B", f'w.activities = ["{other}"];'))
    checks.expect(wait_for(lambda: row() == {"A": 1, "C": 2, "D": 3}),
                  f"moving B to another activity closes its gap instead of leaving an empty column ({row()})")
    switch_to(checks, other)
    checks.expect(wait_for(lambda: row() == {"B": 1}), f"on the other activity B has the row to itself ({row()})")
    checks.expect(wait_for(lambda: active_title() == "B"), f"and has focus ({active_title()})")
    switch_to(checks, home)
    checks.expect(wait_for(lambda: row() == {"A": 1, "C": 2, "D": 3}), f"switching back brings A, C and D back in their order ({row()})")
    run_script(for_window("B", "w.activities = [];"))
    checks.expect(wait_for(lambda: sorted(row()) == ["A", "B", "C", "D"] and sorted(row().values()) == [1, 2, 3, 4]),
                  f"putting B on all activities brings it back into the row ({row()})")


def main():
    manager = subprocess.Popen(["/usr/lib/kactivitymanagerd"])
    try:
        if not wait_for(manager_running, 30):
            print("kactivitymanagerd did not start\nRESULT: FAIL")
            return
        Checks().run(only_current_activity_takes_room)
    finally:
        manager.terminate()
        manager.wait(timeout=10)


if __name__ == "__main__":
    main()
