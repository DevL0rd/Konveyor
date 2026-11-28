#!/usr/bin/env python3
"""Konveyor's own overview paints every workspace at once, pans with the wheel and
moves a window to another workspace by dragging it onto that card."""
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

import virtualtouchpad
from borderprobe import screenshot
from checks import Checks
from fakepointer import Held, move, tap
from keycodes import KEY_CODES
from kwinsession import frame, konveyor, konveyor_action, open_client, qdbus, wait_for

COLORS = {"A": (200, 0, 0), "B": (0, 200, 0), "C": (0, 0, 200)}
LEFT_BUTTON = 0x110
TOLERANCE = 30
SETTLE = 0.6


def close(pixel, color):
    return all(abs(channel - expected) <= TOLERANCE for channel, expected in zip(pixel[:3], color))


def box(image, color, step=4):
    """Bounding box of the pixels that show this client color, or None."""
    pixels = image.load()
    left, top, right, bottom = image.width, image.height, -1, -1
    for row in range(0, image.height, step):
        for column in range(0, image.width, step):
            if close(pixels[column, row], color):
                left, top = min(left, column), min(top, row)
                right, bottom = max(right, column), max(bottom, row)
    return None if right < left else (left, top, right - left, bottom - top)


def boxes(image):
    return {title: box(image, color) for title, color in COLORS.items()}


def overview_open():
    return json.loads(konveyor("OverviewState"))["is_open"]


def kde_overview_active():
    return "overview" in qdbus("org.kde.KWin", "/Effects", "org.freedesktop.DBus.Properties.Get", "org.kde.kwin.Effects",
                               "activeEffects").split()


def workspace_of(title):
    windows = json.loads(konveyor("Windows"))
    workspaces = {workspace["id"]: workspace for workspace in json.loads(konveyor("Workspaces"))}
    window = next(window for window in windows if window["title"] == title)
    return workspaces[window["workspace_id"]]["idx"]


def active_index():
    return next(workspace["idx"] for workspace in json.loads(konveyor("Workspaces")) if workspace["is_active"])


def on_workspace(index):
    return wait_for(lambda: active_index() == index, 20)


def wait_for_boxes(colors=COLORS):
    def seen():
        found = boxes(screenshot())
        return all(found[title] for title in colors)

    return wait_for(seen, 20, 0.2)


def show_workspaces(checks):
    move(960, 540)
    open_client("A", client="colored-client.qml")
    checks.expect(on_workspace(1), "the first workspace is active")
    for title, index in (("B", 2), ("C", 3)):
        checks.equal(konveyor_action("focus-workspace", str(index)), "", f"switch to workspace {index}")
        checks.expect(on_workspace(index), f"workspace {index} is active")
        open_client(title, client="colored-client.qml")
    checks.equal(konveyor_action("focus-workspace", "1"), "", "return to the first workspace")
    checks.expect(on_workspace(1), "the first workspace is active again")
    checks.expect(wait_for(lambda: set(COLORS) <= {window["title"] for window in json.loads(konveyor("Windows"))}, 60),
                  "the three colored windows opened")
    checks.equal(konveyor_action("toggle-overview"), "", "toggle-overview opens Konveyor's overview")
    checks.expect(wait_for(overview_open, 10), "the overview reports itself open")
    checks.expect(wait_for_boxes(), "every workspace shows its window at once")
    checks.expect(not kde_overview_active(), "KDE's own overview stays out of it")
    normal = frame("A")
    image = screenshot()
    found = boxes(image)
    checks.expect(found["A"][2] < normal[2] / 2, f"A is painted smaller than its real frame ({found['A']} vs {normal})")
    for title in ("B", "C"):
        checks.expect(found[title][2] < normal[2] / 2, f"{title} from another workspace is painted too ({found[title]})")


def drag_to_another_workspace(checks):
    found = boxes(screenshot())
    source = (found["A"][0] + found["A"][2] // 2, found["A"][1] + found["A"][3] // 2)
    target = (found["B"][0] + found["B"][2] // 2, found["B"][1] + found["B"][3] // 2)
    before = workspace_of("B")
    with Held() as pointer:
        pointer.send(f"move:{source[0]}:{source[1]}", f"button:{LEFT_BUTTON}:1")
        pointer.glide(source, target, 16)
        time.sleep(SETTLE)
        pointer.send(f"button:{LEFT_BUTTON}:0")
    checks.expect(wait_for(lambda: overview_open(), 5), "dropping a window keeps the overview open")
    tap([], KEY_CODES["Escape"])
    checks.expect(wait_for(lambda: not overview_open(), 10), "Escape closes the overview")
    checks.expect(wait_for(lambda: workspace_of("A") == before, 20), f"A moved onto B's workspace {before} ({workspace_of('A')})")
    checks.expect(workspace_of("A") != 1, "A left the first workspace")


def wheel_pans(checks):
    move(960, 540)
    checks.equal(konveyor_action("toggle-overview"), "", "the overview opens again")
    checks.expect(wait_for_boxes(), "the windows are painted again")
    before = box(screenshot(), COLORS["A"])
    virtualtouchpad.add()
    try:
        virtualtouchpad.scroll(False, 15)
        time.sleep(SETTLE)
        after = box(screenshot(), COLORS["A"])
    finally:
        virtualtouchpad.remove()
    checks.expect(after is not None, "A is still visible after the wheel")
    checks.expect(before[1] - after[1] >= 20, f"the wheel pans the stack up ({before[1]} -> {after[1]})")
    tap([], KEY_CODES["Escape"])
    checks.expect(wait_for(lambda: not overview_open(), 10), "Escape closes the panned overview")


def main():
    checks = Checks()
    checks.run(show_workspaces, drag_to_another_workspace, wheel_pans)


if __name__ == "__main__":
    main()
