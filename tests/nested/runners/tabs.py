#!/usr/bin/env python3
import os
import re
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from fakepointer import click
from kwinsession import active_title, frame, frames, konveyor_action, for_window, run_script, wait_for
from screenshot import capture_workspace

COLORS = {"A": (200, 0, 0), "B": (0, 200, 0), "C": (0, 0, 200), "D": (200, 200, 0)}
ACTIVE = (255, 0, 255)
INACTIVE = (0, 255, 255)
GAPS = 16
BORDER = 4


def use_config(position="left", width=8, gap=5, radius=0, inside=False, border=False):
    lines = [f'position "{position}"', f"width {width}", f"gap {gap}", f"corner-radius {radius}", 'active-color "#ff00ff"',
             'inactive-color "#00ffff"'] + (["place-within-column"] if inside else [])
    block = "    tab-indicator {\n" + "".join(f"        {line}\n" for line in lines) + "    }\n\n    struts {"
    config = re.sub(r"    struts \{", block, default_config(), count=1)
    return load_config(config.replace("    border {\n        off\n", "    border {\n") if border else config)


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
    checks.expect(use_config(), "the tab indicator config loads")
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


def fullscreen_tabs_switch_too(checks):
    konveyor_action("fullscreen-window")
    checks.expect(wait_for(lambda: frame("B") == (0.0, 0.0, 1920.0, 1080.0) and shown("B"), 30, 0.3), "the fullscreen column shows B")
    konveyor_action("focus-window-up")
    checks.expect(wait_for(lambda: active_title() == "A" and shown("A"), 30, 0.3), "switching tabs while fullscreen shows A")
    konveyor_action("focus-window-down")
    checks.expect(wait_for(lambda: active_title() == "B" and shown("B"), 30, 0.3), "switching back while fullscreen shows B")
    konveyor_action("fullscreen-window")
    checks.expect(wait_for(lambda: frame("B")[2] < 1920, 30), "B leaves fullscreen")


def room_beside(title, position):
    x, y, w, h = frame(title)
    rooms = {"left": x, "right": frame("C")[0] - (x + w), "top": y, "bottom": 1080 - (y + h)}
    return rooms[position]


def thick_tabs_get_room(checks):
    for position in ("left", "right", "top", "bottom"):
        checks.expect(use_config(position=position, width=32, gap=5), f"a 32 pixel {position} tab indicator config loads")
        painted = wait_for(lambda: band_painted("B", position, 32, 5), 30, 0.3)
        checks.expect(painted, f"a 32 pixel {position} tab indicator is fully visible beside the window")
        roomy = wait_for(lambda: room_beside("B", position) >= 5 + 32 + 5, 30)
        checks.expect(roomy, f"the window keeps room for a 32 pixel {position} indicator and its distance on both sides ({room_beside('B', position)})")
    checks.expect(use_config(width=32), "a 32 pixel left tab indicator config loads")
    checks.expect(wait_for(lambda: room_beside("B", "left") == 42, 30), "a thick left indicator moves the window right")
    checks.expect(use_config(width=4), "a 4 pixel left tab indicator config loads")
    checks.expect(wait_for(lambda: room_beside("B", "left") == GAPS, 30), "a thin indicator fits in the gap and moves nothing")


def first_tab_corner(title, width, gap):
    x, y, _, height = frame(title)
    image = screenshot()
    left, top = x - gap - width, y + (height - round(height * 0.5)) / 2
    return pixel(image, left + width / 2, top + 1), pixel(image, left + 1, top + 1)


def corner_is(rounded):
    edge, corner = first_tab_corner("B", 16, 5)
    return edge is not None and corner is not None and is_tab(edge) and is_tab(corner) != rounded


def every_side_inside_and_outside(checks):
    for position in ("left", "right", "top", "bottom"):
        for inside in (False, True):
            where = f"{position} {'inside' if inside else 'outside'} the column with borders"
            checks.expect(use_config(position=position, width=12, gap=4, inside=inside, border=True), f"a tab indicator {where} config loads")
            checks.expect(wait_for(lambda: band_painted("B", position, 12, 4 + BORDER), 30, 0.3), f"the tab indicator {where} is fully visible")
    checks.expect(use_config(), "the thin tab indicator config loads again")


def roundness_rounds_the_tabs(checks):
    checks.expect(use_config(width=16, gap=5, radius=8), "a tab indicator config with a corner radius of 8 loads")
    checks.expect(wait_for(lambda: corner_is(rounded=True), 30, 0.3), f"a corner radius of 8 cuts the corners of the tabs {first_tab_corner('B', 16, 5)}")
    checks.expect(use_config(width=16, gap=5, radius=0), "a tab indicator config with a corner radius of 0 loads")
    checks.expect(wait_for(lambda: corner_is(rounded=False), 30, 0.3), f"a corner radius of 0 keeps them square {first_tab_corner('B', 16, 5)}")


def close_window(title):
    run_script(for_window(title, "w.closeWindow();"))
    return wait_for(lambda: frame(title) is None, 30)


def moving_and_closing_tabs(checks):
    konveyor_action("focus-column-right")
    checks.expect(wait_for(lambda: active_title() == "C"), "C is focused")
    konveyor_action("consume-or-expel-window-left")
    checks.expect(wait_for(lambda: tabs_share_frame("A", "B", "C") and shown("C"), 30, 0.3), "C joins the tabs and is shown")
    konveyor_action("consume-or-expel-window-right")
    checks.expect(wait_for(lambda: not tabs_share_frame("A", "C") and shown("C"), 30, 0.3), "C leaves the tabs and stays shown")
    konveyor_action("focus-column-left")
    checks.expect(wait_for(lambda: active_title() in ("A", "B") and shown(active_title()), 30, 0.3), "the tabbed column shows its focused tab")
    konveyor_action("focus-window-bottom")
    checks.expect(wait_for(lambda: active_title() == "B" and shown("B"), 30, 0.3), "B is the shown tab")
    checks.expect(close_window("B"), "B closes")
    checks.expect(wait_for(lambda: shown("A"), 30, 0.3), "closing the shown tab shows A")


def main():
    Checks().run(switching_tabs_changes_the_shown_window, clicking_a_tab_shows_its_window, fullscreen_tabs_switch_too, thick_tabs_get_room, every_side_inside_and_outside, roundness_rounds_the_tabs,
                 moving_and_closing_tabs)


if __name__ == "__main__":
    main()
