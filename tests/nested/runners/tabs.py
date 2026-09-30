#!/usr/bin/env python3
import os
import re
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, config_path, default_config
from fakepointer import click
from kwinsession import active_title, frame, frames, konveyor_action, wait_for
from screenshot import capture_workspace

COLORS = {"A": (200, 0, 0), "B": (0, 200, 0), "C": (0, 0, 200), "D": (200, 200, 0)}
ACTIVE = (255, 0, 255)
INACTIVE = (0, 255, 255)
GAPS = 16


def write_config(position="left", width=8, gap=5, radius=0):
    block = (f'    tab-indicator {{\n        position "{position}"\n        width {width}\n        gap {gap}\n        corner-radius {radius}\n'
             f'        active-color "#ff00ff"\n        inactive-color "#00ffff"\n    }}\n\n    struts {{')
    config_path().write_text(re.sub(r"    struts \{", block, default_config(), count=1))


def screenshot():
    return capture_workspace(tempfile.mktemp(suffix=".png", dir=os.environ["KONVEYOR_TEST_ROOT"]))


def close(pixel, color, tolerance=40):
    return all(abs(a - b) <= tolerance for a, b in zip(pixel[:3], color))


def is_tab(pixel):
    return close(pixel, ACTIVE) or close(pixel, INACTIVE)


def pixel(image, x, y):
    x, y = round(x), round(y)
    return image.getpixel((x, y)) if 0 <= x < image.width and 0 <= y < image.height else None


def shown(title):
    x, y, width, height = frame(title)
    image = screenshot()
    return close(pixel(image, x + width / 2, y + height / 2), COLORS[title])


def tabs_share_frame(*titles):
    placed = frames()
    return len({placed.get(title) for title in titles}) == 1


def tab_band(title, position, width, gap):
    x, y, w, h = frame(title)
    middle = gap + width / 2
    along = (0.3, 0.4, 0.5, 0.6, 0.7)
    points = {"left": [(x - middle, y + h * f) for f in along], "right": [(x + w + middle, y + h * f) for f in along],
              "top": [(x + w * f, y - middle) for f in along], "bottom": [(x + w * f, y + h + middle) for f in along]}
    return points[position]


def band_painted(title, position, width, gap):
    image = screenshot()
    return all(value is not None and is_tab(value) for value in (pixel(image, *point) for point in tab_band(title, position, width, gap)))


def make_tabs():
    konveyor_action("focus-column-first")
    konveyor_action("focus-column-right")
    wait_for(lambda: active_title() == "B")
    konveyor_action("consume-or-expel-window-left")
    konveyor_action("toggle-column-tabbed-display")
    return wait_for(lambda: tabs_share_frame("A", "B"), 30)


def switching_tabs_changes_the_shown_window(checks):
    write_config()
    checks.expect(make_tabs(), "A and B share one tabbed column")
    checks.expect(wait_for(lambda: shown("B"), 30, 0.3), "the focused tab B is shown")
    konveyor_action("focus-window-up")
    checks.expect(wait_for(lambda: active_title() == "A"), "focus moves to tab A")
    checks.expect(wait_for(lambda: shown("A"), 30, 0.3), "switching to tab A shows A")
    konveyor_action("focus-window-down")
    checks.expect(wait_for(lambda: shown("B"), 30, 0.3), "switching back shows B")


def clicking_a_tab_shows_its_window(checks):
    x, y, _, height = frame("A")
    click(x - 5 - 4, y + height * 0.375)
    checks.expect(wait_for(lambda: active_title() == "A" and shown("A"), 30, 0.3), "clicking the first tab shows A")
    click(x - 5 - 4, y + height * 0.625)
    checks.expect(wait_for(lambda: active_title() == "B" and shown("B"), 30, 0.3), "clicking the second tab shows B")
    click(x + 100, y + 100)
    checks.expect(wait_for(lambda: "konveyor-test-clicked:B" in Path(os.environ["KONVEYOR_KWIN_LOG"]).read_text(errors="replace"), 30, 0.3),
                  "a click on the window still reaches it")


def room_beside(title, position):
    x, y, w, h = frame(title)
    rooms = {"left": x, "right": frame("C")[0] - (x + w), "top": y, "bottom": 1080 - (y + h)}
    return rooms[position]


def thick_tabs_get_room(checks):
    for position in ("left", "right", "top", "bottom"):
        write_config(position=position, width=32, gap=5)
        painted = wait_for(lambda: band_painted("B", position, 32, 5), 30, 0.3)
        checks.expect(painted, f"a 32 pixel {position} tab indicator is fully visible beside the window")
        room = room_beside("B", position)
        checks.expect(room >= 5 + 32 + 5, f"the window keeps room for a 32 pixel {position} indicator and its distance on both sides ({room})")
    write_config()
    checks.expect(wait_for(lambda: room_beside("B", "left") == GAPS, 30), "a thin indicator fits in the gap and moves nothing")


def first_tab_corner(title, width, gap):
    x, y, _, height = frame(title)
    image = screenshot()
    left, top = x - gap - width, y + (height - round(height * 0.5)) / 2
    return pixel(image, left + width / 2, top + 1), pixel(image, left + 1, top + 1)


def corner_is(rounded):
    edge, corner = first_tab_corner("B", 16, 5)
    return edge is not None and corner is not None and is_tab(edge) and is_tab(corner) != rounded


def roundness_rounds_the_tabs(checks):
    write_config(width=16, gap=5, radius=8)
    checks.expect(wait_for(lambda: corner_is(rounded=True), 30, 0.3), f"a corner radius of 8 cuts the corners of the tabs {first_tab_corner('B', 16, 5)}")
    write_config(width=16, gap=5, radius=0)
    checks.expect(wait_for(lambda: corner_is(rounded=False), 30, 0.3), f"a corner radius of 0 keeps them square {first_tab_corner('B', 16, 5)}")


def main():
    Checks().run(switching_tabs_changes_the_shown_window, clicking_a_tab_shows_its_window, thick_tabs_get_room, roundness_rounds_the_tabs)


if __name__ == "__main__":
    main()
