#!/usr/bin/env python3
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, config_path, load_config
from fakepointer import move
from kwinsession import activate, for_window, konveyor, konveyor_action, konveyor_windows, open_client, run_script, wait_for

LEFT_CENTER = (400, 540)
RIGHT_CENTER = (2400, 540)
LEFT_EDGE = (0, 540)
RIGHT_EDGE = (3839, 540)
SHARED_LEFT_SIDE = (1919, 540)
SHARED_RIGHT_SIDE = (1920, 540)
DELAY = 0.3
SETTLE = DELAY + 0.7


def engine_focus():
    return next((window.get("title") for window in konveyor_windows() if window.get("is_focused")), None)


def engine_output():
    focused = json.loads(konveyor("FocusedOutput"))
    return focused[0]["name"] if focused else None


def window_output(title):
    printed = run_script(for_window(title, 'print("MARK|" + w.output.name);'))
    return printed[0] if printed else None


def focus(title):
    activate(title)
    return wait_for(lambda: engine_focus() == title, 30)


def cursor():
    printed = run_script('print("MARK|" + workspace.cursorPos.x + "," + workspace.cursorPos.y);')
    return printed[0] if printed else None


def visit(edge, away):
    move(*away, settle=False)
    time.sleep(0.2)
    move(*edge, settle=False)
    print(f"pointer at {cursor()} for {edge}")


def steps_once(checks, edge, away, expected, label):
    visit(edge, away)
    checks.expect(wait_for(lambda: engine_focus() == expected, 5, 0.05), f"{label} focuses {expected} (focused {engine_focus()})")
    time.sleep(SETTLE)
    checks.equal(engine_focus(), expected, f"staying there does not step again ({label})")


def stays(checks, edge, away, expected, label):
    visit(edge, away)
    time.sleep(SETTLE)
    checks.equal(engine_focus(), expected, f"{label} leaves the focus alone")


def setup(checks):
    move(*LEFT_CENTER, settle=False)
    for title in ("A", "B", "C", "D", "E"):
        open_client(title, width=500)
        checks.expect(wait_for(lambda: engine_focus() == title, 60), f"{title} opens and takes focus")
    for title in ("D", "E"):
        checks.expect(focus(title), f"{title} is focused before moving it")
        konveyor_action("move-column-to-monitor-right")
        checks.expect(wait_for(lambda: window_output(title) == "Virtual-1", 30), f"{title} moves to Virtual-1 ({window_output(title)})")
    checks.equal([window_output(title) for title in "ABCDE"], ["Virtual-0"] * 3 + ["Virtual-1"] * 2, "A, B and C sit on Virtual-0, D and E on Virtual-1")


def outer_edges(checks):
    checks.expect(focus("C"), "C is focused")
    move(*LEFT_CENTER, settle=False)
    steps_once(checks, LEFT_EDGE, LEFT_CENTER, "B", "left screen edge")
    steps_once(checks, LEFT_EDGE, LEFT_CENTER, "A", "returning to the left edge")
    checks.expect(focus("D"), "D is focused")
    move(*RIGHT_CENTER, settle=False)
    steps_once(checks, RIGHT_EDGE, RIGHT_CENTER, "E", "right screen edge")


def passing_by(checks):
    checks.expect(focus("A"), "A is focused")
    move(*LEFT_CENTER, settle=False)
    move(*LEFT_EDGE, settle=False)
    time.sleep(0.05)
    move(*LEFT_CENTER, settle=False)
    time.sleep(SETTLE)
    checks.equal(engine_focus(), "A", "touching the edge for less than the delay does nothing")


def shared_edges_outer(checks):
    checks.expect(focus("A"), "A is focused")
    stays(checks, SHARED_LEFT_SIDE, LEFT_CENTER, "A", "the right edge of Virtual-0, shared with Virtual-1,")
    stays(checks, SHARED_RIGHT_SIDE, RIGHT_CENTER, "A", "the left edge of Virtual-1, shared with Virtual-0,")
    checks.equal(engine_output(), "Virtual-0", "the focused output stays Virtual-0")


def shared_edges_all(checks):
    text = config_path().read_text()
    checks.expect(load_config(text.replace('monitor-edges "outer"', 'monitor-edges "all"')), 'monitor-edges "all" loads')
    checks.expect(focus("A"), "A is focused")
    steps_once(checks, SHARED_LEFT_SIDE, LEFT_CENTER, "B", "the shared right edge of Virtual-0")
    steps_once(checks, SHARED_RIGHT_SIDE, RIGHT_CENTER, "D", "the shared left edge of Virtual-1 focuses that output and")
    checks.equal(engine_output(), "Virtual-1", "the focused output follows to Virtual-1")
    checks.expect(load_config(text), "the outer-only config is back")


def fullscreen(checks):
    checks.expect(focus("B"), "B is focused")
    run_script(for_window("B", "w.fullScreen = true;"))
    checks.expect(wait_for(lambda: run_script(for_window("B", 'print("MARK|" + w.fullScreen);')) == ["true"], 10), "B is fullscreen")
    stays(checks, LEFT_EDGE, LEFT_CENTER, "B", "with a fullscreen window focused the left edge")
    run_script(for_window("B", "w.fullScreen = false;"))
    checks.expect(wait_for(lambda: run_script(for_window("B", 'print("MARK|" + w.fullScreen);')) == ["false"], 10), "B leaves fullscreen")


def switched_off(checks):
    text = config_path().read_text()
    checks.expect(load_config(text.replace("edge-switch-columns { on;", "edge-switch-columns { off;")), "edge-switch-columns off loads")
    checks.expect(focus("B"), "B is focused")
    stays(checks, LEFT_EDGE, LEFT_CENTER, "B", "with the option off the left edge")
    checks.expect(load_config(text), "the enabled config is back")


if __name__ == "__main__":
    Checks().run(setup, outer_edges, passing_by, shared_edges_outer, shared_edges_all, fullscreen, switched_off)
