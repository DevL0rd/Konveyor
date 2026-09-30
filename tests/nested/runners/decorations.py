#!/usr/bin/env python3
import os
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import activate, active_title, frame, konveyor_action, wait_for
from screenshot import capture_workspace

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


def close(pixel, color, tolerance=48):
    return all(abs(a - b) <= tolerance for a, b in zip(pixel[:3], color))


def rounded_frame(title):
    return tuple(round(value) for value in frame(title))


def ring_pixel(image, title):
    x, y, width, height = rounded_frame(title)
    if x - RING < 0 or x + width > image.width:
        return None
    return image.getpixel((x - RING // 2, y + height // 2))


def ring_is(title, color):
    pixel = ring_pixel(screenshot(), title)
    return pixel is not None and close(pixel, color)


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
    subprocess.run(["kwriteconfig6", "--notify", "--file", "kdeglobals", "--group", "Colors:Selection", "--key", "BackgroundNormal", "0,0,255"], check=True)
    checks.expect(wait_for(lambda: ring_is(focused, BLUE), 30, 0.5), f"changing the KDE accent recolours the ring live ({ring_pixel(screenshot(), focused)})")


def tab_pixels(title):
    image = screenshot()
    x, y, width, height = rounded_frame(title)
    column_x = x - TAB_GAP - TAB_WIDTH // 2
    return {image.getpixel((column_x, row))[:3] for row in range(y, y + height, 4)} if column_x >= 0 else set()


def tab_bar(checks):
    activate("B")
    checks.equal(konveyor_action("consume-or-expel-window-left"), "", "stack B under A")
    checks.equal(konveyor_action("toggle-column-tabbed-display"), "", "show the column as tabs")
    checks.expect(wait_for(lambda: any(close(pixel, MAGENTA) for pixel in tab_pixels("B")), 30, 0.5), "the active tab is magenta")
    checks.expect(wait_for(lambda: any(close(pixel, CYAN) for pixel in tab_pixels("B")), 30, 0.5), "the other tab is cyan")


def main():
    Checks().run(focus_ring, accent_change, tab_bar)


if __name__ == "__main__":
    main()
