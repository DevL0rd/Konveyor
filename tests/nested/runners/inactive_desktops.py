#!/usr/bin/env python3
import json
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

import virtualtouchpad as touchpad
from checks import Checks
from fakepointer import move
from kwinsession import frame, intersects, konveyor, konveyor_action, open_client, run_script, wait_for
from screenshot import capture_workspace

RED = "red"
GREEN = "green"
LEAST_PIXELS = 80


def screen():
    x, y, width, height = (float(value) for value in run_script('const g = workspace.activeScreen.geometry; print("MARK|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height);')[0].split("|"))
    return (x, y, width, height)


def on_screen(title):
    placed = frame(title)
    return placed is not None and intersects(placed, screen())


def overview_open():
    return json.loads(konveyor("OverviewState"))["is_open"]


def active_workspace():
    return next(workspace["idx"] for workspace in json.loads(konveyor("Workspaces")) if workspace["is_active"])


def screenshot(name):
    return capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).parent / f"{name}.png"))


def count_color(image, color):
    small = image.convert("RGB").resize((image.width // 4, image.height // 4))
    data = small.tobytes()
    matched = 0
    for index in range(0, len(data), 3):
        red, green, blue = data[index], data[index + 1], data[index + 2]
        if color == RED:
            matched += red > 90 and red > green * 2 and red > blue * 2
        else:
            matched += green > 90 and green > red * 2 and green > blue * 2
    return matched


def painted(image, color):
    return count_color(image, color) >= LEAST_PIXELS


def wait_painted(name, color, present=True, timeout=30):
    return wait_for(lambda: painted(screenshot(name), color) == present, timeout, 0.2)


def inactive_windows(checks):
    checks.equal(konveyor_action("focus-workspace", "1"), "", "start on the first workspace")
    open_client("A", client="colored-client.qml")
    checks.expect(wait_for(lambda: on_screen("A")), "the red window is on the first workspace")
    konveyor_action("focus-workspace", "2")
    checks.expect(wait_for(lambda: active_workspace() == 2), "switch to the second workspace")
    open_client("B", client="colored-client.qml")
    checks.expect(wait_for(lambda: on_screen("B")), "the green window is on the second workspace")
    konveyor_action("focus-workspace", "1")
    checks.expect(wait_for(lambda: active_workspace() == 1), "return to the first workspace")
    time.sleep(1.5)

    home = frame("B")
    checks.expect(wait_for(lambda: on_screen("B")), "a window on an inactive desktop stays on screen")
    checks.expect(wait_painted("without-overview", RED), "the active desktop's red window is painted")
    checks.expect(wait_painted("without-overview", GREEN, False, 5), "the inactive desktop's green window is not painted")

    checks.equal(konveyor_action("open-overview"), "", "open the overview")
    checks.expect(wait_for(overview_open), "the overview is open")
    checks.expect(wait_painted("overview", GREEN), "the overview paints the inactive desktop's window")
    checks.expect(on_screen("B"), "the overview leaves the inactive desktop's window on screen")
    checks.equal(frame("B"), home, "the inactive desktop's window keeps its local frame in the overview")

    konveyor_action("close-overview")
    checks.expect(wait_for(lambda: not overview_open()), "the overview closes")
    checks.equal(frame("B"), home, "the local frame is unchanged after the overview")
    checks.expect(wait_painted("after-overview", GREEN, False), "the inactive desktop's window is hidden again after the overview")


def swipe_paints_source_and_destination(checks):
    checks.equal(active_workspace(), 1, "start on the first workspace")
    checks.expect(wait_painted("before-swipe", RED), "the red source window paints before the swipe")
    checks.expect(wait_painted("before-swipe", GREEN, False, 5), "the green destination window stays hidden before the swipe")

    move(960, 540)
    touchpad.add()
    touchpad.swipe_begin(3)
    for _ in range(5):
        touchpad.swipe_update(0, -40)
    checks.expect(wait_for(lambda: painted(screenshot("mid-swipe"), RED) and painted(screenshot("mid-swipe"), GREEN), 10, 0.2),
                  "both the source and destination windows paint mid-swipe")
    for _ in range(5):
        touchpad.swipe_update(0, -40)

    touchpad.swipe_end()
    checks.expect(wait_for(lambda: active_workspace() == 2), "the swipe settles on the second workspace")
    checks.expect(wait_painted("settled", GREEN), "the destination window paints after the swipe")
    checks.expect(wait_painted("settled", RED, False), "the source window is hidden after the swipe")
    touchpad.remove()


def keyboard_switch_paints_destination(checks):
    checks.equal(konveyor_action("focus-workspace", "2"), "", "start on the second workspace")
    checks.expect(wait_for(lambda: active_workspace() == 2), "the second workspace is active")
    checks.equal(konveyor_action("focus-workspace", "1"), "", "switch to the first workspace with a keyboard action")
    checks.expect(wait_for(lambda: active_workspace() == 1), "the keyboard action changes the active workspace")
    checks.expect(wait_painted("keyboard-switch", RED), "the destination window paints after a keyboard switch")
    checks.expect(wait_painted("keyboard-switch", GREEN, False), "the source window is hidden after a keyboard switch")


def main():
    Checks().run(inactive_windows, swipe_paints_source_and_destination, keyboard_switch_paints_destination)


if __name__ == "__main__":
    main()
