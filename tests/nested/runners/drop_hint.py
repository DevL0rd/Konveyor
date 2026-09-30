#!/usr/bin/env python3
import os
import sys
import tempfile
import time
from contextlib import contextmanager
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from fakepointer import Held, move
from kwinsession import activate, frame, konveyor_windows, wait_for
from screenshot import capture_workspace

META = 125
LEFT_BUTTON = 0x110
MAGENTA = (255, 0, 255)
WINDOW = (47, 48, 51)
DRAGGED_OPACITY = 0.75


def blend(over, under, alpha):
    return tuple(round(alpha * a + (1 - alpha) * b) for a, b in zip(over, under))


def hint_area(color=MAGENTA):
    return painted(screenshot(), color, blend(WINDOW, color, DRAGGED_OPACITY))


def screenshot():
    return capture_workspace(tempfile.mktemp(suffix=".png", dir=os.environ["KONVEYOR_TEST_ROOT"])).convert("RGB")


def close(pixel, color, tolerance=16):
    return all(abs(a - b) <= tolerance for a, b in zip(pixel[:3], color))


def painted(image, *colors, step=4):
    points = [(x, y) for y in range(0, image.height, step) for x in range(0, image.width, step)
              if any(close(image.getpixel((x, y)), color) for color in colors)]
    if not points:
        return None
    xs, ys = [x for x, _ in points], [y for _, y in points]
    return min(xs), min(ys), max(xs) + step, max(ys) + step


def settled(title):
    previous = [None]

    def still():
        current, previous[0] = frame(title), frame(title)
        return current == previous[0] and current

    time.sleep(0.3)
    return wait_for(still, 10, 0.3)


def center(title):
    x, y, width, height = frame(title)
    return round(x + width / 2), round(y + height / 2)


def placement(title):
    window = next(window for window in konveyor_windows() if window["title"] == title)
    return window["layout"]["pos_in_scrolling_layout"]


@contextmanager
def dragging(title, goal, grab=None):
    start = grab or center(title)
    lifted = (start[0], start[1] - 300 if start[1] > 540 else start[1] + 300)
    with Held() as pointer:
        pointer.send((META, 1), f"move:{start[0]}:{start[1]}", f"button:{LEFT_BUTTON}:1")
        pointer.glide(start, lifted, 8)
        pointer.glide(lifted, goal, 16)
        yield pointer
        pointer.send(f"button:{LEFT_BUTTON}:0", (META, 0))
    move(960, 1070)


def hint_for_new_column(checks):
    activate("B")
    target = settled("B")
    goal = (round(target[0] + target[2] + 150), 540)
    beside = lambda area: area and abs(area[0] - (target[0] + target[2] + 16)) <= 24 and abs(area[2] - area[0] - 300) <= 24
    with dragging("C", goal, grab=(round(frame("C")[0] + 40), 540)):
        checks.expect(wait_for(lambda: beside(hint_area()), 10, 0.5), f"a column-wide drop hint is drawn right of B ({hint_area()}, B at {target})")
    checks.expect(placement("C")[1] == 1 and placement("C")[0] == placement("B")[0] + 1, f"C dropped into its own column after B ({placement('C')})")


def hint_over_column_top(checks):
    activate("B")
    target = settled("B")
    goal = (round(target[0] + target[2] / 2), round(target[1] + 20))
    column = placement("B")[0]
    along_top = lambda area: area and area[1] <= target[1] + 4 and abs(area[3] - target[1] - 150) <= 24 and area[2] - area[0] >= target[2] - 24
    with dragging("C", goal):
        checks.expect(wait_for(lambda: along_top(hint_area()), 10, 0.5), f"a drop hint is drawn along the top of B while C is dragged there ({hint_area()}, B at {target})")
        through = (goal[0] + 200, goal[1] + 60)
        see_through = blend(WINDOW, MAGENTA, DRAGGED_OPACITY)
        checks.expect(wait_for(lambda: close(screenshot().getpixel(through), see_through), 10, 0.5),
                      f"the dragged window is drawn see-through over the hint ({screenshot().getpixel(through)} at {through})")
    checks.expect(wait_for(lambda: hint_area() is None, 10, 0.5), "the drop hint is gone after the drop")
    checks.equal(placement("C"), [column, 1], "C dropped at the top of B's column")
    checks.equal(placement("B"), [column, 2], "B moved below C")


def main():
    Checks().run(hint_for_new_column, hint_over_column_top)


if __name__ == "__main__":
    main()
