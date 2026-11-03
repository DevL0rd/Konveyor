#!/usr/bin/env python3
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from borderprobe import MAGENTA, TITLES, painted_titles, probe_config, screenshot, settle_to, solid
from checks import Checks, load_config
from kwinsession import frame, konveyor_action, qdbus, wait_for

PROBE = 'border-style "Probe" {\n    layer "gradient" {\n        colors "#ff00ff"\n        speed 0\n        width 6\n        drive "width" "%s"\n    }\n}\n'
RING = 'width 4\n        active-style "Probe"\n        inactive-style "Probe"'
STEP = 'border-style "Probe" {\n    layer "particles" { colors "#ff00ff"; fire-on "view.step"; count 8; width 6; }\n}\n'
SLOW_ROW = 'horizontal-view-movement { duration-ms 6000; curve "linear"; }'
PAST_THE_HOLD = 1.5


def windows():
    return {window["title"]: window for window in json.loads(qdbus("org.kde.Konveyor", "/Konveyor", "org.kde.Konveyor.SignalFrames"))["windows"]}


def value(title, name):
    return windows()[title]["values"][name]


def fired(title, name):
    return windows()[title]["fired"][name]




def fill_the_screen(checks):
    for title in TITLES:
        settle_to(title)
        checks.equal(konveyor_action("set-column-width", "60%"), "", f"{title} takes sixty percent of the screen")
    settle_to("A")


def sample(seconds):
    taken = []
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        window = windows()["B"]
        taken.append({**window["values"], **{"fired." + name: count for name, count in window["fired"].items()}})
    return taken


def row_moves(checks, styles, ring, action):
    checks.expect(load_config(probe_config("", styles, ring=ring)), "the config loads")
    settle_to("A")
    time.sleep(1.0)
    before = sample(0.1)[-1]
    checks.equal(konveyor_action(action), "", f"{action}")
    return before, sample(2.5)


def unreferenced_view_signals_stay_zero(checks):
    before, taken = row_moves(checks, solid("Probe", "#ff00ff"), RING, "focus-column-right")
    checks.expect(all(entry["view.distance"] == before["view.distance"] and entry["view.speed"] == 0 for entry in taken),
                  "nothing publishes the view signals while no style or effect uses them")
    checks.expect(all(entry["fired.view.step"] == before["fired.view.step"] for entry in taken), "and no view step is counted")


def each_view_signal_wakes_the_tracker(checks):
    before, taken = row_moves(checks, PROBE % "view.speed", RING, "focus-column-right")
    checks.expect(any(entry["view.speed"] > 0 for entry in taken), "a style using view.speed sees the row move")
    checks.expect(taken[-1]["view.speed"] == 0, "and the speed settles to zero once the row stops")
    checks.expect(all(entry["view.direction"] == 0 and entry["view.distance"] == 0 for entry in taken),
                  "the direction and distance nobody asked for stay unpublished")
    before, taken = row_moves(checks, PROBE % "view.direction", RING, "focus-column-right")
    checks.expect(any(entry["view.direction"] == 1 for entry in taken), "a style using view.direction reads a positive direction moving right")
    konveyor_action("focus-column-left")
    checks.expect(any(entry["view.direction"] == -1 for entry in sample(2.5)), "and a negative one moving left")
    before, taken = row_moves(checks, PROBE % "view.distance", RING, "focus-column-right")
    checks.expect(any(entry["view.distance"] != before["view.distance"] for entry in taken), "a style using view.distance sees the row travel")


def view_steps_fire_per_column(checks):
    before, taken = row_moves(checks, STEP, RING, "focus-column-right")
    checks.expect(taken[-1]["fired.view.step"] > before["fired.view.step"], "changing the active column counts a view step")
    count = taken[-1]["fired.view.step"]
    konveyor_action("focus-column-right")
    checks.expect(wait_for(lambda: sample(0.1)[-1]["fired.view.step"] > count, 10), "and the next column another")


def row_at_rest():
    x = frame("A")[0]
    time.sleep(0.3)
    return frame("A")[0] == x


def nothing_painted():
    return not painted_titles(screenshot(), TITLES, MAGENTA)


def view_effect_plays_while_the_row_moves(checks):
    events = '        on "view" "Mark" reach="all"'
    mark = solid("Mark", "#ff00ff")
    checks.expect(load_config(probe_config(events, mark)), "a view effect loads")
    settle_to("A")
    checks.expect(wait_for(row_at_rest, 20), "the row comes to rest")
    checks.expect(wait_for(nothing_painted, 15, 0.3), "and a resting row plays nothing")
    checks.expect(load_config(probe_config(events, mark, animations=SLOW_ROW)), "a slow row loads")
    konveyor_action("focus-column-right")
    started = time.monotonic()

    def painted_while_moving_past_the_hold():
        x = frame("A")[0]
        painted_now = not nothing_painted()
        return painted_now and time.monotonic() - started > PAST_THE_HOLD and frame("A")[0] != x

    if checks.expect(wait_for(painted_while_moving_past_the_hold, 10, 0.02), "scrolling the row keeps the view effect playing"):
        checks.expect(wait_for(lambda: row_at_rest() and nothing_painted(), 20, 0.3), "and it ends after the row stops")
    checks.expect(load_config(probe_config("", mark, animations=SLOW_ROW)), "the config without it loads")
    settle_to("A")
    checks.expect(not wait_for(lambda: not nothing_painted(), 3, 0.2), "without a view effect nothing plays")
    checks.expect(wait_for(row_at_rest, 20), "and the row comes to rest")


def the_tracker_follows_the_row_across_workspaces(checks):
    checks.expect(load_config(probe_config("", PROBE % "view.distance", ring=RING)), "a style using view.distance loads")
    settle_to("A")
    checks.equal(konveyor_action("focus-workspace-down"), "", "go to the workspace below")
    time.sleep(1.5)
    checks.equal(konveyor_action("focus-workspace-up"), "", "and come back")
    time.sleep(1.5)
    before = sample(0.1)[-1]
    konveyor_action("focus-column-right")
    checks.expect(any(entry["view.distance"] != before["view.distance"] for entry in sample(2.5)), "the row is tracked again on the workspace it returned to")


def main():
    Checks().run(fill_the_screen, unreferenced_view_signals_stay_zero, each_view_signal_wakes_the_tracker, view_steps_fire_per_column,
                 view_effect_plays_while_the_row_moves, the_tracker_follows_the_row_across_workspaces)


if __name__ == "__main__":
    main()
