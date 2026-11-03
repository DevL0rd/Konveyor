#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from fakepointer import move
from kwinsession import activate, active_title, for_window, frame, frames, konveyor_windows, qdbus, run_script, wait_for

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


def hovering_past_a_window_of_another_activity(checks):
    other = activities("AddActivity", "Elsewhere")
    checks.expect(wait_for(lambda: other in run_script('print("MARK|" + workspace.activities.join(","));')[0]), "KWin knows the new activity")
    checks.expect(load_config(default_config().replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    activate("B")
    run_script(for_window("B", f'w.activities = ["{other}"];'))
    checks.expect(wait_for(lambda: "B" not in row()), "B left for the other activity")
    activate("A")
    checks.expect(wait_for(lambda: len(shown()) >= 2), f"another window is on screen ({shown()})")
    target = next(title for title in shown() if title != "A")
    checks.expect(wait_for(lambda: frame(target) == tuple(settled(target))), f"{target} stops moving")
    x, y, width, height = frame(target)
    move(round(x + width / 2), round(y + height / 2))
    checks.expect(wait_for(lambda: active_title() == target, 10), f"hovering {target} focuses it past a window of another activity ({active_title()})")
    run_script(for_window("B", "w.activities = [];"))
    load_config(default_config())


def shown():
    return sorted(title for title, (x, y, width, height) in frames().items() if title in ("A", "B", "C", "D") and 0 <= x < 1500 and title in row())


def settled(title):
    layout = next(window["layout"] for window in konveyor_windows() if window["title"] == title)
    return tuple(round(value) for value in (*layout["tile_pos_in_workspace_view"], *layout["tile_size"]))


def main():
    manager = subprocess.Popen(["/usr/lib/kactivitymanagerd"])
    try:
        if not wait_for(manager_running, 30):
            print("kactivitymanagerd did not start\nRESULT: FAIL")
            return
        Checks().run(only_current_activity_takes_room, hovering_past_a_window_of_another_activity)
    finally:
        manager.terminate()
        manager.wait(timeout=10)


if __name__ == "__main__":
    main()
