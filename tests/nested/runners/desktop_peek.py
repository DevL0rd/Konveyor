#!/usr/bin/env python3
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from fakepointer import Held, chord, move
from keycodes import KEY_CODES, MODIFIER_CODES
from kwinsession import activate, frame, konveyor_action, konveyor_windows, managed_titles, marked_lines, qdbus, wait_for
from nested import build_dir
from screenshot import capture_workspace, close

SUPER = MODIFIER_CODES["Super"]
RIGHT = KEY_CODES["Right"]
LEFT_BUTTON = 0x110
CLIENT = (0x2F, 0x30, 0x33)
HOLD = 0.3
SETTLE = 0.8
SHOTS = Path(os.environ["KONVEYOR_TEST_ROOT"]) / "shots"
LAUNCHER = """registerShortcut("KonveyorPeekLauncher", "Peek test launcher", "Meta", function () { print("PEEKLAUNCHER|fired"); });"""


def launcher_count():
    return len(marked_lines("PEEKLAUNCHER|"))


def log_count(text):
    with open(os.environ["KONVEYOR_KWIN_LOG"], errors="replace") as log:
        return sum(1 for line in log if text in line)


def peek_count():
    return log_count("konveyor: peeking at the desktop")


def engine_focus():
    return next((window.get("title") for window in konveyor_windows() if window.get("is_focused")), None)


def center(rect):
    return int(rect[0] + rect[2] / 2), int(rect[1] + rect[3] / 2)


def body(rect):
    left, right = max(rect[0], 0), min(rect[0] + rect[2], 1920)
    return (int((left + right) / 2), int(rect[1] + rect[3] * 0.8)) if right - left > 40 else None


def placed(title):
    rect = frame(title)
    return tuple(round(value) for value in rect) if rect else None


def shows_client(image, point):
    return close(image.getpixel(point), CLIENT)


def shot(name):
    SHOTS.mkdir(exist_ok=True)
    return capture_workspace(SHOTS / f"{name}.png")


def register_launcher(checks):
    script = SHOTS.parent / "peek-launcher.js"
    script.write_text(LAUNCHER)
    script_id = qdbus("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting.loadScript", str(script), "peek-launcher")
    qdbus("org.kde.KWin", f"/Scripting/Script{script_id}", "org.kde.kwin.Script.run")
    time.sleep(1.0)
    before = launcher_count()
    chord([SUPER])
    checks.expect(wait_for(lambda: launcher_count() == before + 1, 5), "a Meta tap fires the modifier-only test shortcut")


def setup(checks):
    checks.expect(activate("B"), "B is focused")
    konveyor_action("set-column-width", "80%")
    checks.expect(wait_for(lambda: placed("B") and placed("B")[2] > 1400, 10), f"B is wider than half the screen ({frame('B')})")
    konveyor_action("center-column")
    time.sleep(SETTLE)
    register_launcher(checks)


def tap(checks):
    before = {title: placed(title) for title in "ABC"}
    launched, peeks = launcher_count(), peek_count()
    chord([SUPER])
    checks.expect(wait_for(lambda: launcher_count() == launched + 1, 5), "a quick Meta tap still fires the launcher shortcut")
    time.sleep(SETTLE)
    checks.equal(peek_count(), peeks, "a quick tap does not peek")
    checks.equal({title: placed(title) for title in "ABC"}, before, "a quick tap moves nothing")


def hold_and_slide(checks):
    layout = placed("B")
    others = [placed(title) for title in "AC"]
    visible = [rect for rect in others if body(rect)]
    checks.expect(len(visible) > 0, f"another window shows beside B ({others})")
    launched, peeks = launcher_count(), peek_count()
    move(*center(layout))
    image = shot("before")
    checks.expect(all(shows_client(image, body(rect)) for rect in visible), "the other windows show before the peek")
    with Held() as held:
        held.send((SUPER, 1))
        checks.expect(wait_for(lambda: peek_count() == peeks + 1, 3), "holding Meta alone starts a peek")
        time.sleep(SETTLE)
        image = shot("peeking")
        checks.expect(not any(shows_client(image, body(rect)) for rect in visible), "the other windows faded out")
        checks.expect(shows_client(image, body(layout)), "the focused window stays")
        checks.equal(placed("B"), layout, "the focused window has not moved while the pointer is on it")
        held.send("move:2:540")
        half = round((1920 - 3 * 16) / 2)
        aside = (1920 - 16 - half, layout[1], half, layout[3])
        checks.expect(wait_for(lambda: placed("B") == aside, 5), f"leaving B on the left slides it right at half width ({placed('B')} vs {aside})")
        time.sleep(SETTLE)
        image = shot("aside")
        checks.expect(not shows_client(image, (200, 540)) and shows_client(image, body(aside)), "the desktop shows where B was and B sits aside")
        held.send(f"move:{aside[0] + 100}:540", "move:2:540", f"move:{aside[0] + 100}:540")
        time.sleep(SETTLE)
        checks.equal(placed("B"), aside, "B moves once per peek, even when the pointer crosses it again")
        held.send((SUPER, 0))
    checks.expect(wait_for(lambda: placed("B") == layout, 5), f"releasing Meta puts B back ({placed('B')} vs {layout})")
    time.sleep(SETTLE)
    image = shot("restored")
    checks.expect(all(shows_client(image, body(rect)) for rect in visible), "the other windows are back")
    checks.equal([placed(title) for title in "AC"], others, "the other windows did not move")
    checks.equal(launcher_count(), launched, "releasing Meta after a peek does not fire the launcher shortcut")
    chord([SUPER])
    checks.expect(wait_for(lambda: launcher_count() == launched + 1, 5), "the next quick tap fires the launcher again")


def shortcut_during_delay(checks):
    checks.expect(activate("B"), "B is focused")
    time.sleep(SETTLE)
    launched, peeks = launcher_count(), peek_count()
    with Held() as held:
        held.send((SUPER, 1), (RIGHT, 1), (RIGHT, 0))
        time.sleep(HOLD + SETTLE)
        held.send((SUPER, 0))
    checks.expect(wait_for(lambda: engine_focus() == "C", 5), f"Meta+Right still focuses the next column ({engine_focus()})")
    checks.equal(peek_count(), peeks, "Meta+Right before the delay does not peek")
    checks.equal(launcher_count(), launched, "Meta+Right does not fire the launcher shortcut")


def click_during_delay(checks):
    checks.expect(activate("B"), "B is focused")
    peeks = peek_count()
    with Held() as held:
        held.send((SUPER, 1), "move:2:540", "button:272:1", "button:272:0")
        time.sleep(HOLD + SETTLE)
        held.send((SUPER, 0))
    checks.equal(peek_count(), peeks, "a click before the delay cancels the peek")


def fullscreen(checks):
    checks.expect(activate("B"), "B is focused")
    peeks = peek_count()
    konveyor_action("fullscreen-window")
    checks.expect(wait_for(lambda: placed("B") == (0, 0, 1920, 1080), 10), f"B is fullscreen ({placed('B')})")
    with Held() as held:
        held.send((SUPER, 1))
        time.sleep(HOLD + SETTLE)
        held.send((SUPER, 0))
    checks.equal(peek_count(), peeks, "a fullscreen window does not peek")
    konveyor_action("fullscreen-window")


def drag_during_peek(checks):
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    subprocess.Popen([str(build_dir() / "bin" / "drag_source_client"), "S"], stdout=log, stderr=subprocess.STDOUT)
    checks.expect(wait_for(lambda: "S" in managed_titles(), 60), "the drag source opened")
    checks.expect(activate("S"), "S is focused")
    time.sleep(SETTLE)
    layout = placed("S")
    start = center(layout)
    peeks, over, drags = peek_count(), log_count("konveyor: peek over"), log_count("konveyor-test-drag-finished")
    move(*start)
    with Held() as held:
        held.send((SUPER, 1))
        checks.expect(wait_for(lambda: peek_count() == peeks + 1, 3), "holding Meta alone starts a peek")
        held.send(f"button:{LEFT_BUTTON}:1")
        held.glide(start, (start[0] + 60, start[1]), 6)
        checks.expect(wait_for(lambda: log_count("konveyor-test-drag-started") > 0, 5), "a press during the peek reaches S and starts a drag")
        held.glide((start[0] + 60, start[1]), (4, 540))
        time.sleep(SETTLE + 0.7)
        checks.equal(placed("S"), layout, "nothing moves while a file is dragged, even at the screen edge")
        held.send((SUPER, 0))
        time.sleep(SETTLE)
        checks.equal(log_count("konveyor: peek over"), over, "letting go of Meta mid-drag keeps the peek until the drop")
        image = shot("dragging")
        checks.expect(not shows_client(image, (200, 540)), "the other windows stay faded during the drag")
        held.send(f"button:{LEFT_BUTTON}:0")
    checks.expect(wait_for(lambda: log_count("konveyor-test-drag-finished") > drags, 5), "the drop finishes")
    checks.expect(wait_for(lambda: log_count("konveyor: peek over") == over + 1, 5), "the drop ends the peek")
    checks.expect(wait_for(lambda: placed("S") == layout, 5), f"S is where it was ({placed('S')} vs {layout})")


if __name__ == "__main__":
    Checks().run(setup, tap, hold_and_slide, shortcut_during_delay, click_during_delay, fullscreen, drag_during_peek)
