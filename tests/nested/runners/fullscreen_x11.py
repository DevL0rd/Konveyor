#!/usr/bin/env python3
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import activate, active_title, konveyor_action, kwin_titles, wait_for, watch_changes, window_state
from screenshot import capture_workspace, close

TITLE = "X11Game"
FULLSCREEN = "true|0,0 1920x1080"
WHITE = (255, 255, 255, 255)
COLUMN_COLOR = (47, 48, 51, 255)


def overlay_problems():
    problems = []
    image = capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).with_suffix(".png")))
    column = [int(v) for v in window_state("A").split("|")[1].replace(" ", ",").replace("x", ",").split(",")]
    center = image.getpixel((column[0] + column[2] // 2, column[1] + column[3] // 2))
    right_edge = image.getpixel((image.width - 4, image.height // 2))
    print(f"drawn while unfocused: center of A {center}, right edge {right_edge}")
    if not close(center, COLUMN_COLOR):
        problems.append(f"the focused column is not drawn over the fullscreen window ({center})")
    if right_edge != WHITE:
        problems.append(f"the fullscreen window did not stay in place behind the columns ({right_edge})")
    return problems


def covered_colors():
    image = capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).with_suffix(".back.png")))
    return [image.getpixel((x, image.height // 2)) for x in range(0, image.width, 40)]


def main():
    problems = []
    wait_for(lambda: TITLE in kwin_titles(), 60)
    wait_for(lambda: window_state(TITLE) == FULLSCREEN, 60)
    activate(TITLE)
    wait_for(lambda: window_state(TITLE) == FULLSCREEN, 60)
    focused = window_state(TITLE)
    print(f"focused: {focused}")
    if focused != FULLSCREEN:
        problems.append(f"focused fullscreen window does not cover the output ({focused})")
    activate("A")
    if active_title() != "A":
        problems.append(f"could not focus A away from the fullscreen window (active: {active_title()})")
    changes = watch_changes(TITLE, 3)
    unfocused = window_state(TITLE)
    print(f"unfocused: {unfocused}, changes in 3s: {len(changes)} {changes[:6]}")
    if changes:
        problems.append(f"fullscreen window keeps changing while unfocused ({len(changes)} changes in 3s)")
    if unfocused != FULLSCREEN:
        problems.append(f"unfocused fullscreen window left its output geometry ({unfocused})")
    wait_for(lambda: not overlay_problems(), 60)
    problems += overlay_problems()
    konveyor_action("focus-column-right")
    wait_for(lambda: window_state(TITLE) == FULLSCREEN and active_title() == TITLE, 60)
    back = window_state(TITLE)
    print(f"back on the fullscreen window: {back}, active {active_title()}")
    if back != FULLSCREEN or active_title() != TITLE:
        problems.append(f"moving back did not focus the fullscreen window over the output ({back})")
    wait_for(lambda: all(color == WHITE for color in covered_colors()), 60)
    covered = covered_colors()
    print(f"visible colors after moving back: {sorted(set(covered))}")
    if any(color != WHITE for color in covered):
        problems.append("columns are still visible over the fullscreen window after moving back to it")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
