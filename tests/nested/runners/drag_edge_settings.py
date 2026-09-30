#!/usr/bin/env python3
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, load_config
from dragging import LEFT_BUTTON, META, center, drag_config, placement, settled
from fakepointer import Held, move
from keycodes import KEY_CODES
from kwinsession import activate, konveyor_windows, wait_for

ROWS = [
    ("trigger-width 0 turns edge scrolling off", "trigger-width 0", 4, [(1.5, 0, 0)]),
    ("a wider trigger-width scrolls further from the edge", "trigger-width 400", 300, [(1.5, 150, None)]),
    ("delay-ms holds off scrolling for its delay", "delay-ms 2500", 4, [(1.0, 0, 0), (3.0, 150, None)]),
    ("max-speed caps how fast the row scrolls", "max-speed 100", 4, [(1.5, 1, 250)]),
]


def view_x(title):
    return next(window["layout"]["tile_pos_in_workspace_view"][0] for window in konveyor_windows() if window["title"] == title)


def with_edge_scroll(settings):
    return drag_config().replace("gestures {\n", "gestures {\n    dnd-edge-view-scroll { " + settings + "; }\n", 1)


def edge_settings(checks):
    for label, settings, edge_x, holds in ROWS:
        checks.expect(load_config(with_edge_scroll(settings)), f"{settings} loads")
        activate("D")
        settled("D")
        start = center("D")
        with Held() as pointer:
            pointer.send((META, 1), f"move:{start[0]}:{start[1]}", f"button:{LEFT_BUTTON}:1")
            pointer.glide(start, (edge_x, start[1]))
            before = view_x("A")
            held = 0.0
            for seconds, least, most in holds:
                time.sleep(seconds - held)
                held = seconds
                scrolled = view_x("A") - before
                within = scrolled >= least and (most is None or scrolled <= most)
                checks.expect(within, f"{label}: after {seconds}s at x={edge_x} the row scrolled {scrolled:.0f}px, expected {least}..{most}")
            pointer.send((KEY_CODES["Escape"], 1), (KEY_CODES["Escape"], 0), f"button:{LEFT_BUTTON}:0", (META, 0))
        move(960, 1070)
        checks.expect(wait_for(lambda: placement("D")[0] == 4), f"Escape puts D back in the last column ({placement('D')})")


def main():
    Checks().run(edge_settings)


if __name__ == "__main__":
    main()
