#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from fakepointer import keys
from kwinsession import activate, for_window, konveyor_action, konveyor_windows, run_script, wait_for

META = 125
RIGHT_BUTTON = 0x111
DRAG = 120


def frame(title):
    printed = run_script(for_window(title, 'const g = w.frameGeometry; print("MARK|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height);'))
    return tuple(round(float(value)) for value in printed[0].split("|"))


def tile(title):
    layout = next(window["layout"] for window in konveyor_windows() if window["title"] == title)
    return tuple(round(value) for value in (*layout["tile_pos_in_workspace_view"], *layout["tile_size"]))


def settled(title):
    return frame(title) == tile(title)


def drag(start, end):
    keys((META, 1), f"move:{start[0]}:{start[1]}", f"button:{RIGHT_BUTTON}:1", *(f"move:{start[0] + (end[0] - start[0]) * step / 8}:{start[1] + (end[1] - start[1]) * step / 8}"
         for step in range(1, 9)), f"button:{RIGHT_BUTTON}:0", (META, 0))


def resized(checks, title, grab, offset, dimension, label):
    activate(title)
    checks.expect(wait_for(lambda: settled(title)), f"{title} is in place before dragging its {label} edge")
    x, y, width, height = frame(title)
    start = grab(x, y, width, height)
    end = (start[0] + offset[0], start[1] + offset[1])
    checks.expect(0 <= end[0] < 1920 and 0 <= end[1] < 1080, f"the drag from {start} to {end} stays on screen")
    drag(start, end)
    before = (width, height)[dimension]
    grown = wait_for(lambda: settled(title) and frame(title)[2 + dimension] >= before + DRAG - 2)
    checks.expect(grown, f"dragging the {label} edge outwards by {DRAG}px grows {title} (from {before} to {frame(title)[2 + dimension]})")


def edges(checks):
    resized(checks, "B", lambda x, y, w, h: (x + 12, y + h // 2), (-DRAG, 0), 0, "left")
    resized(checks, "B", lambda x, y, w, h: (x + w - 12, y + h // 2), (DRAG, 0), 0, "right")
    activate("C")
    checks.equal(konveyor_action("consume-or-expel-window-left"), "", "stack C under B")
    resized(checks, "C", lambda x, y, w, h: (x + w // 2, y + 12), (0, -DRAG), 1, "top")
    resized(checks, "B", lambda x, y, w, h: (x + w // 2, y + h - 12), (0, DRAG), 1, "bottom")


def main():
    Checks().run(edges)


if __name__ == "__main__":
    main()
