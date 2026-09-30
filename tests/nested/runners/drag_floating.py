#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from dragging import dragging, placement, settled, window_json
from drophint import hint_area
from kwinsession import activate, frame, konveyor_action, wait_for

ESCAPE = 1
RIGHT_BUTTON = 0x111


def right_click(pointer):
    pointer.send(f"button:{RIGHT_BUTTON}:1", f"button:{RIGHT_BUTTON}:0")


def top_of(title):
    target = settled(title)
    return round(target[0] + target[2] / 2), round(target[1] + 20)


def contains(area, point):
    return area[0] <= point[0] < area[0] + area[2] and area[1] <= point[1] < area[1] + area[3]


def floating(title):
    return window_json(title)["is_floating"]


def tile_again(checks, title):
    activate(title)
    checks.equal(konveyor_action("move-window-to-tiling"), "", f"tile {title} again")
    checks.expect(wait_for(lambda: not floating(title), 10), f"{title} is tiled")


def expel(checks, title):
    activate(title)
    checks.equal(konveyor_action("consume-or-expel-window-right"), "", f"give {title} its own column again")
    checks.expect(wait_for(lambda: len({placement(each)[0] for each in ("A", "B", "C")}) == 3, 10), f"A, B and C have their own columns ({[placement(each) for each in ("A", "B", "C")]}, {floating(title)})")


def right_click_floats_a_tiled_drag(checks):
    activate("B")
    with dragging("C", (700, 500)) as pointer:
        checks.expect(wait_for(lambda: hint_area() is not None, 10, 0.5), "the tiled drag shows a drop hint")
        right_click(pointer)
        checks.expect(wait_for(lambda: hint_area() is None, 10, 0.5), f"a right click turns the drag floating and the hint goes away ({hint_area()})")
        pointer.glide((700, 500), (760, 520), 4)
    checks.expect(wait_for(lambda: floating("C"), 10), "C drops as a floating window")
    checks.expect(contains(settled("C"), (760, 520)), f"C floats under the spot it was dropped at ({frame('C')})")
    tile_again(checks, "C")


def right_click_twice_keeps_it_tiled(checks):
    activate("B")
    column = placement("B")[0]
    goal = top_of("B")
    with dragging("C", goal) as pointer:
        right_click(pointer)
        checks.expect(wait_for(lambda: hint_area() is None, 10, 0.5), "the first right click takes the hint away")
        right_click(pointer)
        checks.expect(wait_for(lambda: hint_area() is not None, 10, 0.5), "the second right click brings it back")
    checks.expect(not floating("C"), "C stays tiled")
    checks.equal(placement("C"), [column, 1], "C stacks on top of B where the hint was")
    expel(checks, "C")


def right_click_tiles_a_floating_drag(checks):
    activate("C")
    checks.equal(konveyor_action("toggle-window-floating"), "", "float C")
    settled("C")
    activate("B")
    column = placement("B")[0]
    activate("C")
    goal = top_of("B")
    with dragging("C", goal) as pointer:
        checks.expect(hint_area() is None, f"the floating drag has no hint ({hint_area()})")
        right_click(pointer)
        checks.expect(wait_for(lambda: hint_area() is not None, 10, 0.5), "a right click turns the drag tiled and shows the hint")
    checks.expect(wait_for(lambda: not floating("C"), 10), "C drops as a tiled window")
    checks.equal(placement("C"), [column, 1], "C stacks on top of B")
    expel(checks, "C")


def binds_switch_the_drag(checks):
    activate("B")
    with dragging("C", (700, 500)):
        checks.equal(konveyor_action("toggle-window-floating"), "", "toggle-window-floating mid-drag")
        checks.expect(wait_for(lambda: hint_area() is None, 10, 0.5), "the hint goes away")
    checks.expect(wait_for(lambda: floating("C"), 10), "C drops floating")
    goal = top_of("B")
    column = placement("B")[0]
    with dragging("C", goal):
        checks.equal(konveyor_action("move-window-to-tiling"), "", "move-window-to-tiling mid-drag")
        checks.expect(wait_for(lambda: hint_area() is not None, 10, 0.5), "the hint shows")
        checks.equal(konveyor_action("move-window-to-tiling"), "", "asking again changes nothing")
        checks.expect(hint_area() is not None, "the hint is still there")
    checks.expect(wait_for(lambda: not floating("C"), 10), "C drops tiled")
    checks.equal(placement("C"), [column, 1], "on top of B")
    expel(checks, "C")
    with dragging("C", (700, 500)):
        checks.equal(konveyor_action("move-window-to-floating"), "", "move-window-to-floating mid-drag")
        checks.expect(wait_for(lambda: hint_area() is None, 10, 0.5), "the hint goes away")
    checks.expect(wait_for(lambda: floating("C"), 10), "C drops floating")
    tile_again(checks, "C")


def escape_after_right_click(checks):
    activate("B")
    before = {title: placement(title) for title in ("A", "B", "C")}
    frame_before = settled("C")
    with dragging("C", (700, 500)) as pointer:
        right_click(pointer)
        checks.expect(wait_for(lambda: hint_area() is None, 10, 0.5), "the drag turned floating")
        pointer.send((ESCAPE, 1), (ESCAPE, 0))
    checks.expect(wait_for(lambda: not floating("C"), 10), "Escape puts C back as a tiled window")
    checks.equal({title: placement(title) for title in ("A", "B", "C")}, before, "every window is back where it was")
    checks.equal(settled("C"), frame_before, "C is back in its own spot")


def floating_size_comes_back(checks):
    activate("C")
    checks.equal(konveyor_action("toggle-window-floating"), "", "float C")
    checks.equal(konveyor_action("set-window-width", "500"), "", "make the floating C 500 wide")
    checks.equal(konveyor_action("set-window-height", "400"), "", "and 400 high")
    checks.expect(wait_for(lambda: settled("C")[2:] == (500.0, 400.0), 10), f"C floats at 500x400 ({frame('C')})")
    tile_again(checks, "C")
    checks.expect(settled("C")[2:] != (500.0, 400.0), f"the tiled C has its column size ({frame('C')})")
    activate("B")
    with dragging("C", (700, 500)) as pointer:
        right_click(pointer)
        checks.expect(wait_for(lambda: hint_area() is None, 10, 0.5), "the drag turned floating")
    checks.expect(wait_for(lambda: floating("C") and settled("C")[2:] == (500.0, 400.0), 10), f"C drops at its floating size ({frame('C')})")
    tile_again(checks, "C")


def main():
    Checks().run(right_click_floats_a_tiled_drag, right_click_twice_keeps_it_tiled, right_click_tiles_a_floating_drag, binds_switch_the_drag,
                 escape_after_right_click, floating_size_comes_back)


if __name__ == "__main__":
    main()
