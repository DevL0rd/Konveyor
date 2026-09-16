#!/usr/bin/env python3
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import activate, window_state
from screenshot import capture_workspace

TITLE = "SelfFullscreen"
FULLSCREEN = "true|0,0 1920x1080"
FULLSCREEN_COLOR = (176, 48, 48, 255)
COLUMN_COLOR = (47, 48, 51, 255)


def check_overlay(problems):
    image = capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).with_suffix(".png")))
    middle = image.height // 2
    left_edge = image.getpixel((4, middle))
    right_side = image.getpixel((image.width - 40, middle))
    column_left = next(x for x in range(image.width) if image.getpixel((x, middle)) == COLUMN_COLOR)
    beside = image.getpixel((column_left - 40, middle))
    edge_gap = image.getpixel((image.width - 4, middle))
    print(f"drawn while unfocused: left edge {left_edge}, right side {right_side}, beside the column {beside}, edge gap {edge_gap}")
    if left_edge != FULLSCREEN_COLOR or right_side != COLUMN_COLOR:
        problems.append(f"column A is not drawn over the fullscreen window that stays in place ({left_edge}, {right_side})")
    if not beside[0] < FULLSCREEN_COLOR[0] - 40:
        problems.append(f"the fullscreen window is not shaded beside the overlaid column ({beside})")
    if not edge_gap[0] < FULLSCREEN_COLOR[0] - 60:
        problems.append(f"the fullscreen window shows unshaded in the gap under the overlaid columns ({edge_gap})")


def main():
    problems = []
    time.sleep(0.5)
    activate(TITLE)
    time.sleep(3)
    for sample in range(4):
        state = window_state(TITLE)
        print(f"sample {sample}: {state}")
        if state != FULLSCREEN:
            problems.append(f"sample {sample}: window is not fullscreen on the output ({state})")
        time.sleep(1)
    activate("A")
    time.sleep(1)
    unfocused = [window_state(TITLE) for _ in range(5)]
    print(f"while unfocused: {unfocused}")
    if any(state != FULLSCREEN for state in unfocused):
        problems.append(f"window did not stay fullscreen and still while unfocused ({unfocused})")
    check_overlay(problems)
    time.sleep(4)
    state = window_state(TITLE)
    print(f"after leaving fullscreen: {state}")
    if not state or not state.startswith("false|") or " 1920x1080" in state:
        problems.append(f"window did not return to its column after leaving fullscreen ({state})")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
