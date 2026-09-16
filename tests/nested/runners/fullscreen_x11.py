#!/usr/bin/env python3
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import activate, active_title, konveyor_action, watch_changes, window_state
from screenshot import capture_workspace

TITLE = "X11Game"
FULLSCREEN = "true|0,0 1920x1080"
WHITE = (255, 255, 255, 255)
COLUMN_COLOR = (47, 48, 51, 255)


def check_overlay(problems):
    image = capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).with_suffix(".png")))
    column = [int(v) for v in window_state("A").split("|")[1].replace(" ", ",").replace("x", ",").split(",")]
    center = image.getpixel((column[0] + column[2] // 2, column[1] + column[3] // 2))
    left_edge = image.getpixel((4, image.height // 2))
    print(f"drawn while unfocused: center of A {center}, left edge {left_edge}")
    if center != COLUMN_COLOR:
        problems.append(f"the focused column is not drawn over the fullscreen window ({center})")
    if left_edge != WHITE:
        problems.append(f"the fullscreen window did not stay in place behind the columns ({left_edge})")


def main():
    problems = []
    time.sleep(4)
    activate(TITLE)
    time.sleep(2)
    focused = window_state(TITLE)
    print(f"focused: {focused}")
    if focused != FULLSCREEN:
        problems.append(f"focused fullscreen window does not cover the output ({focused})")
    activate("A")
    time.sleep(2)
    if active_title() != "A":
        problems.append(f"could not focus A away from the fullscreen window (active: {active_title()})")
    changes = watch_changes(TITLE, 3)
    unfocused = window_state(TITLE)
    print(f"unfocused: {unfocused}, changes in 3s: {len(changes)} {changes[:6]}")
    if changes:
        problems.append(f"fullscreen window keeps changing while unfocused ({len(changes)} changes in 3s)")
    if unfocused != FULLSCREEN:
        problems.append(f"unfocused fullscreen window left its output geometry ({unfocused})")
    check_overlay(problems)
    konveyor_action("focus-column-left")
    time.sleep(2)
    back = window_state(TITLE)
    print(f"back on the fullscreen window: {back}, active {active_title()}")
    if back != FULLSCREEN or active_title() != TITLE:
        problems.append(f"moving back did not focus the fullscreen window over the output ({back})")
    image = capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).with_suffix(".back.png")))
    covered = [image.getpixel((x, image.height // 2)) for x in range(0, image.width, 40)]
    print(f"visible colors after moving back: {sorted(set(covered))}")
    if any(color != WHITE for color in covered):
        problems.append("columns are still visible over the fullscreen window after moving back to it")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
