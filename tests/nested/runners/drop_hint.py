#!/usr/bin/env python3
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, config_path
from dragging import dragging, placement, quick_tiling, settled, window_json
from drophint import DRAGGED_OPACITY, MAGENTA, WINDOW, blend, close, hint_area, screenshot
from kwinsession import activate, frame, frames, konveyor, konveyor_action, wait_for

GREEN = (0, 255, 0)
HINT_WIDTH = 300
GAP = 16


def use_hint(block):
    text = re.sub(r"    insert-hint \{.*?\n    \}\n", "    insert-hint {\n" + block + "    }\n", config_path().read_text(), count=1, flags=re.S)
    config_path().write_text(text)
    return konveyor("LoadConfigFile", "")


def new_column_goal():
    activate("B")
    target = settled("B")
    return target, (round(target[0] + target[2] + HINT_WIDTH / 2), 540)


def left_edge_grab():
    return round(frame("C")[0] + 40), 540


def pixel_in_new_column_hint(target, offset):
    return screenshot().getpixel((round(target[0] + target[2] + GAP + offset), 540))


def hint_for_new_column(checks):
    target, goal = new_column_goal()
    beside = lambda area: area and abs(area[0] - (target[0] + target[2] + GAP)) <= 24 and abs(area[2] - area[0] - HINT_WIDTH) <= 24
    with dragging("C", goal, grab=left_edge_grab()):
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
    activate("C")
    checks.equal(konveyor_action("consume-or-expel-window-right"), "", "expel C back into its own column")
    checks.expect(wait_for(lambda: placement("C") == [column + 1, 1]), f"C has its own column again ({placement('C')})")


def hint_follows_window_corners(checks):
    target, goal = new_column_goal()
    left = round(target[0] + target[2] + GAP)
    with dragging("C", goal, grab=left_edge_grab()):
        checks.expect(wait_for(lambda: close(screenshot().getpixel((left + 30, 540)), MAGENTA), 10, 0.5), "the hint is drawn")
        corner = screenshot().getpixel((left + 2, round(target[1]) + 2))
        checks.expect(not close(corner, MAGENTA), f"the hint's corner is rounded like C's 40 px corners ({corner})")


def hint_color_with_alpha(checks):
    checks.equal(use_hint('        color "#ff00ff80"\n'), "", "make the hint half transparent")
    target, goal = new_column_goal()
    half = blend(MAGENTA, (0, 0, 0), 128 / 255)
    with dragging("C", goal, grab=left_edge_grab()):
        checks.expect(wait_for(lambda: close(pixel_in_new_column_hint(target, 30), half, 24), 10, 0.5),
                      f"the half transparent hint lets the background through ({pixel_in_new_column_hint(target, 30)}, want {half})")


def hint_gradient(checks):
    checks.equal(use_hint('        gradient from="#ff0000" to="#0000ff" angle=90\n'), "", "paint the hint with a gradient")
    target, goal = new_column_goal()
    with dragging("C", goal, grab=left_edge_grab()):
        reddish = lambda: (lambda p: p[0] > 180 and p[2] < 80)(pixel_in_new_column_hint(target, 8))
        checks.expect(wait_for(reddish, 10, 0.5), f"the left edge of the hint is red ({pixel_in_new_column_hint(target, 8)})")
        right = pixel_in_new_column_hint(target, HINT_WIDTH - 8)
        checks.expect(right[2] > right[0] + 20, f"the right edge, under the see-through window, is blue ({right})")


def hint_accent(checks):
    checks.equal(use_hint('        color "accent"\n'), "", "follow the KDE accent colour")
    target, goal = new_column_goal()
    with dragging("C", goal, grab=left_edge_grab()):
        checks.expect(wait_for(lambda: close(pixel_in_new_column_hint(target, 30), GREEN), 10, 0.5),
                      f"the hint takes the green accent ({pixel_in_new_column_hint(target, 30)})")


def with_opacity_rule(opacity):
    rule = f'\nwindow-rule {{\n    match title="^C$"\n    opacity {opacity}\n}}\n'
    config_path().write_text(config_path().read_text() + rule)
    return konveyor("LoadConfigFile", "")


def without_opacity_rule():
    config_path().write_text(re.sub(r'\nwindow-rule \{\n    match title="\^C\$"\n    opacity [0-9.]+\n\}\n', "", config_path().read_text()))
    return konveyor("LoadConfigFile", "")


def hint_ignores_window_opacity(opacity):
    def step(checks):
        checks.equal(with_opacity_rule(opacity), "", f"give C a window rule opacity of {opacity}")
        target, goal = new_column_goal()
        under = (round(target[0] + target[2] + GAP + HINT_WIDTH - 8), 540)
        with dragging("C", goal, grab=left_edge_grab()):
            checks.expect(wait_for(lambda: close(pixel_in_new_column_hint(target, 30), MAGENTA), 10, 0.5),
                          f"the hint keeps its own colour ({pixel_in_new_column_hint(target, 30)})")
            through = blend(WINDOW, MAGENTA, DRAGGED_OPACITY * opacity)
            checks.expect(wait_for(lambda: close(screenshot().getpixel(under), through), 10, 0.5),
                          f"C is drawn with its own opacity over the hint ({screenshot().getpixel(under)}, want {through})")
            checks.equal(konveyor_action("toggle-window-rule-opacity"), "", "ignore C's opacity rule mid-drag")
            checks.expect(wait_for(lambda: close(screenshot().getpixel(under), blend(WINDOW, MAGENTA, DRAGGED_OPACITY)), 10, 0.5),
                          f"C is drawn with only the drag fade ({screenshot().getpixel(under)})")
            checks.expect(close(pixel_in_new_column_hint(target, 30), MAGENTA), f"the hint keeps its colour ({pixel_in_new_column_hint(target, 30)})")
        activate("C")
        checks.equal(konveyor_action("toggle-window-rule-opacity"), "", "use C's opacity rule again")
        checks.equal(without_opacity_rule(), "", "drop C's opacity rule")
    step.__name__ = f"hint_ignores_window_opacity_{str(opacity).replace('.', '_')}"
    return step


def hint_off(checks):
    checks.equal(use_hint('        off\n        color "#ff00ff"\n'), "", "turn the hint off")
    target, goal = new_column_goal()
    with dragging("C", goal, grab=left_edge_grab()):
        checks.expect(wait_for(lambda: settled("C")[0] > target[0] + target[2], 10), "C follows the pointer")
        checks.expect(hint_area() is None, f"no hint is drawn ({hint_area()})")
    checks.expect(placement("C")[0] == placement("B")[0] + 1, f"C still drops where the hint would have been ({placement('C')})")
    checks.equal(use_hint('        color "#ff00ff"\n'), "", "turn the hint back on")


def hint_at_left_edge(checks):
    activate("A")
    settled("A")
    half_shown = lambda area: area and area[2] - area[0] >= HINT_WIDTH / 2 - 8
    with dragging("B", (2, 540)):
        checks.expect(wait_for(lambda: half_shown(hint_area()), 10, 0.5), f"a hint for a new first column shows at least half on screen ({hint_area()})")
        checks.expect(all(half_shown(hint_area()) for _ in range(4)), f"it stays at least half on screen while the row scrolls ({hint_area()})")
        checks.equal(sorted(frames()), ["A", "B", "C"], "KWin shows no quick tiling outline at the screen edge")
        checks.equal(quick_tiling(), "false|false", "KWin's quick tiling is held off while a window is dragged")
    checks.equal(placement("B"), [1, 1], "B becomes the first column")
    checks.equal(quick_tiling(), "true|true", "KWin's quick tiling comes back after the drop")


def floating_drag_has_no_hint(checks):
    activate("C")
    checks.equal(konveyor_action("toggle-window-floating"), "", "float C")
    settled("C")
    with dragging("C", (900, 500)):
        checks.expect(wait_for(lambda: abs(sum(settled("C")[:2]) - sum(frame("C")[:2])) < 1 and window_json("C")["is_floating"], 10), "C is dragged floating")
        checks.expect(hint_area() is None, f"a floating window gets no drop hint ({hint_area()})")
    checks.expect(window_json("C")["is_floating"], "C stays floating after the drop")
    activate("C")
    checks.equal(konveyor_action("toggle-window-floating"), "", "tile C again")


def main():
    Checks().run(hint_for_new_column, hint_over_column_top, hint_follows_window_corners,
                 hint_ignores_window_opacity(0.5), hint_ignores_window_opacity(0), hint_color_with_alpha, hint_gradient, hint_accent, hint_off, hint_at_left_edge, floating_drag_has_no_hint)


if __name__ == "__main__":
    main()
