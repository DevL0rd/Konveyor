#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from dragging import dragging, placement, settled
from kwinsession import activate, for_window, frame, konveyor_action, run_script, wait_for, window_state

FULL = (0.0, 0.0, 1920.0, 1080.0)


def fullscreen_by_app(title):
    run_script(for_window(title, "w.fullScreen = true;"))


def fullscreen_by_bind(title):
    konveyor_action("fullscreen-window")


def leave_by_app(title):
    run_script(for_window(title, "w.fullScreen = false;"))


def leave_by_bind(title):
    activate(title)
    konveyor_action("fullscreen-window")


def is_fullscreen(title):
    return window_state(title).startswith("true|") and frame(title) == FULL


def tiled_request(request, leave):
    def step(checks):
        activate("B")
        target = settled("B")
        column = placement("B")[0] + 1
        goal = (round(target[0] + target[2] + 150), 540)
        with dragging("C", goal, grab=(round(frame("C")[0] + 40), 540)) as pointer:
            request("C")
            checks.expect(wait_for(lambda: is_fullscreen("C"), 10), f"C goes fullscreen as soon as it is asked to mid-drag ({window_state('C')})")
            pointer.glide(goal, (goal[0] + 300, goal[1] + 300), 6)
            checks.expect(is_fullscreen("C"), f"moving the pointer on no longer moves C ({window_state('C')})")
        checks.expect(wait_for(lambda: is_fullscreen("C"), 10), f"C stays fullscreen after the button is let go ({window_state('C')})")
        checks.equal(placement("C"), [column, 1], "C was dropped where its hint was, in a new column after B")
        leave("C")
        checks.expect(wait_for(lambda: window_state("C").startswith("false|") and settled("C") != FULL, 10), "C leaves fullscreen")
        checks.equal(placement("C"), [column, 1], "C is back in the column it was dropped into")
    step.__name__ = f"tiled_{request.__name__}"
    return step


def floating_request(request, leave):
    def step(checks):
        activate("C")
        checks.equal(konveyor_action("toggle-window-floating"), "", "float C")
        size = settled("C")[2:]
        with dragging("C", (700, 500)) as pointer:
            dropped = settled("C")
            request("C")
            checks.expect(wait_for(lambda: is_fullscreen("C"), 10), f"the floating C goes fullscreen as soon as it is asked to mid-drag ({window_state('C')})")
            pointer.glide((700, 500), (900, 700), 6)
            checks.expect(is_fullscreen("C"), f"moving the pointer on no longer moves C ({window_state('C')})")
        leave("C")
        checks.expect(wait_for(lambda: window_state("C").startswith("false|") and settled("C")[:2] == dropped[:2], 10),
                      f"C floats again where it was dragged to ({frame('C')}, dragged to {dropped})")
        if request is fullscreen_by_bind:
            checks.equal(frame("C")[2:], size, "C floats again at its own size")
        activate("C")
        checks.equal(konveyor_action("toggle-window-floating"), "", "tile C again")
    step.__name__ = f"floating_{request.__name__}"
    return step


def windowed_fullscreen(checks):
    activate("B")
    target = settled("B")
    with dragging("C", (round(target[0] + target[2] / 2), 540)) as pointer:
        checks.equal(konveyor_action("toggle-windowed-fullscreen"), "", "ask for windowed fullscreen mid-drag")
        still = settled("C")
        pointer.glide((round(target[0] + target[2] / 2), 540), (1500, 900), 6)
        checks.expect(wait_for(lambda: settled("C") == still, 5), f"the drag ends, so C no longer follows the pointer ({frame('C')}, was {still})")
    activate("C")
    checks.equal(konveyor_action("toggle-windowed-fullscreen"), "", "leave windowed fullscreen")


def main():
    Checks().run(tiled_request(fullscreen_by_app, leave_by_app), tiled_request(fullscreen_by_bind, leave_by_bind), windowed_fullscreen,
                 floating_request(fullscreen_by_bind, leave_by_bind), floating_request(fullscreen_by_app, leave_by_app))


if __name__ == "__main__":
    main()
