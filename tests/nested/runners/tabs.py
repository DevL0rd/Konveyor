#!/usr/bin/env python3
import os
import re
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, config_path, default_config
from kwinsession import active_title, frame, frames, konveyor_action, wait_for
from screenshot import capture_workspace

COLORS = {"A": (200, 0, 0), "B": (0, 200, 0), "C": (0, 0, 200), "D": (200, 200, 0)}


def write_config(position="left", width=8, gap=5):
    block = (f'    tab-indicator {{\n        position "{position}"\n        width {width}\n        gap {gap}\n'
             f'        active-color "#ff00ff"\n        inactive-color "#00ffff"\n    }}\n\n    struts {{')
    config_path().write_text(re.sub(r"    struts \{", block, default_config(), count=1))


def screenshot():
    return capture_workspace(tempfile.mktemp(suffix=".png", dir=os.environ["KONVEYOR_TEST_ROOT"]))


def close(pixel, color, tolerance=40):
    return all(abs(a - b) <= tolerance for a, b in zip(pixel[:3], color))


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


def main():
    Checks().run(switching_tabs_changes_the_shown_window)


if __name__ == "__main__":
    main()
