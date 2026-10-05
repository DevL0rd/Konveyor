#!/usr/bin/env python3
import os
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from kwinsession import activate, active_title, frame, konveyor_action, wait_for, watch_signals
from screenshot import capture_workspace, close

RED = (255, 0, 0)
BLUE = (0, 0, 255)
GREEN = (0, 255, 0)
MAGENTA = (255, 0, 255)
CYAN = (0, 255, 255)
RING = 8
TAB_GAP = 4
TAB_WIDTH = 6


def screenshot():
    return capture_workspace(tempfile.mktemp(suffix=".png", dir=os.environ["KONVEYOR_TEST_ROOT"]))


TOLERANCE = 48


def rounded_frame(title):
    return tuple(round(value) for value in frame(title))


def ring_pixel(image, title):
    x, y, width, height = rounded_frame(title)
    if x - RING < 0 or x + width > image.width or not 0 <= y + height // 2 < image.height:
        return None
    return image.getpixel((x - RING // 2, y + height // 2))


def ring_is(title, color):
    pixel = ring_pixel(screenshot(), title)
    return pixel is not None and close(pixel, color, TOLERANCE)


def on_screen(title):
    x, _, width, _ = frame(title)
    return x - RING >= 0 and x + width + RING <= 1920


def neighbour(title):
    return next(other for other in ("A", "B", "C") if other != title and on_screen(other))


def focus_ring(checks):
    activate("B")
    checks.expect(wait_for(lambda: ring_is("B", RED), 30, 0.5), f"the focused window has the red accent ring ({ring_pixel(screenshot(), 'B')})")
    other = neighbour("B")
    checks.expect(not ring_is(other, RED) and not ring_is(other, GREEN), f"an unfocused window has no ring ({ring_pixel(screenshot(), other)})")
    activate(other)
    checks.expect(wait_for(lambda: ring_is(other, RED) and not ring_is("B", RED), 30, 0.5), "the ring follows focus")


def accent_change(checks):
    focused = active_title()
    subprocess.run(["kwriteconfig6", "--file", "kdeglobals", "--group", "Colors:Selection", "--key", "BackgroundNormal", "0,0,255"], check=True)
    notify = ["gdbus", "emit", "--session", "--object-path", "/kdeglobals", "--signal", "org.kde.kconfig.notify.ConfigChanged",
              "{'Colors:Selection': [b'BackgroundNormal']}"]
    announced = watch_signals("type='signal',path='/kdeglobals',interface='org.kde.kconfig.notify',member='ConfigChanged'",
                              lambda: subprocess.run(notify, check=True), lambda lines: any("Colors:Selection" in line for line in lines))
    checks.expect(any("Colors:Selection" in line for line in announced), f"the accent change is announced on the session bus ({announced})")
    kdeglobals = (Path(os.environ["XDG_CONFIG_HOME"]) / "kdeglobals").read_text()
    checks.expect(wait_for(lambda: ring_is(focused, BLUE), 30, 0.5),
                  f"changing the KDE accent recolours the ring live ({ring_pixel(screenshot(), focused)}, kdeglobals {kdeglobals!r})")


def tab_pixels(title):
    image = screenshot()
    x, y, width, height = rounded_frame(title)
    column_x = x - TAB_GAP - TAB_WIDTH // 2
    if not 0 <= column_x < image.width:
        return set()
    return {image.getpixel((column_x, row))[:3] for row in range(max(y, 0), min(y + height, image.height), 4)}


def tab_bar(checks):
    activate("B")
    checks.equal(konveyor_action("consume-or-expel-window-left"), "", "stack B under A")
    checks.equal(konveyor_action("toggle-column-tabbed-display"), "", "show the column as tabs")
    checks.expect(wait_for(lambda: any(close(pixel, MAGENTA, TOLERANCE) for pixel in tab_pixels("B")), 30, 0.5), "the active tab is magenta")
    checks.expect(wait_for(lambda: any(close(pixel, CYAN, TOLERANCE) for pixel in tab_pixels("B")), 30, 0.5), "the other tab is cyan")


PURPLE = (128, 0, 128)
BORDER = 8
DEFAULT_BORDER = """    border {
        off
        width 4
        active-color "accent"
        inactive-color "#505050"
        urgent-color "#9b0000"
    }
"""


def gradient_border(relative_to):
    gradient = f'from="#ff0000" to="#0000ff" angle=90 relative-to="{relative_to}"'
    border = f"    border {{\n        width {BORDER}\n        active-gradient {gradient}\n        inactive-gradient {gradient}\n    }}\n"
    if DEFAULT_BORDER not in default_config():
        raise RuntimeError("the default config no longer has the border block this test rewrites")
    return default_config().replace(DEFAULT_BORDER, border)


def left_border(image, title):
    x, y, _, height = rounded_frame(title)
    return image.getpixel((x - BORDER // 2, y + height // 2))


def border_gradients(checks):
    activate("B")
    checks.equal(konveyor_action("toggle-column-tabbed-display"), "", "show the column as a stack again")
    checks.equal(konveyor_action("consume-or-expel-window-right"), "", "give B its own column again")
    activate("A")
    for relative_to, expected in (("window", {"A": RED, "B": RED}), ("workspace-view", {"A": RED, "B": PURPLE})):
        checks.expect(load_config(gradient_border(relative_to)), f"a gradient border relative to the {relative_to} loads")
        checks.expect(wait_for(lambda: 0 < frame("A")[0] < 100 and 900 < frame("B")[0] < 1000), f"A and B sit side by side at the start of the row ({frame('A')}, {frame('B')})")

        def matches():
            image = screenshot()
            return all(close(left_border(image, title), color, TOLERANCE) for title, color in expected.items())

        checks.expect(wait_for(matches, 30, 0.5),
                      f"relative-to {relative_to}: the left borders of A and B are {expected} ({[left_border(screenshot(), title) for title in expected]})")


def main():
    Checks().run(focus_ring, accent_change, tab_bar, border_gradients)


if __name__ == "__main__":
    main()
