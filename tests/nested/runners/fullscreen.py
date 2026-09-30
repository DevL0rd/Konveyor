#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import CLIENTS, activate, kwin_titles, wait_for, window_state
from screenshot import capture_workspace

TITLE = "SelfFullscreen"
FULLSCREEN = "true|0,0 1920x1080"
FULLSCREEN_COLOR = (176, 48, 48, 255)
COLUMN_COLOR = (47, 48, 51, 255)


def overlay_problems():
    problems = []
    image = capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).with_suffix(".png")))
    middle = image.height // 2
    column = [int(value) for value in window_state("A").split("|")[1].replace(" ", ",").replace("x", ",").split(",")]
    column_center = image.getpixel((column[0] + column[2] // 2, column[1] + column[3] // 2))
    right_side = image.getpixel((image.width - 40, middle))
    beside = image.getpixel((min(image.width - 1, column[0] + column[2] + 40), middle))
    edge_gap = image.getpixel((4, middle))
    print(f"drawn while unfocused: column center {column_center}, right side {right_side}, beside the column {beside}, edge gap {edge_gap}")
    if column_center != COLUMN_COLOR or right_side != FULLSCREEN_COLOR:
        problems.append(f"column A is not drawn over the fullscreen window that stays in place ({column_center}, {right_side})")
    if not beside[0] < FULLSCREEN_COLOR[0] - 40:
        problems.append(f"the fullscreen window is not shaded beside the overlaid column ({beside})")
    if not edge_gap[0] < FULLSCREEN_COLOR[0] - 60:
        problems.append(f"the fullscreen window shows unshaded in the gap under the overlaid columns ({edge_gap})")
    return problems


def main():
    problems = []
    signals = Path(os.environ["KONVEYOR_TEST_ROOT"]) / "signals"
    signals.mkdir()
    with open(os.environ["KONVEYOR_KWIN_LOG"], "a") as log:
        subprocess.Popen(["qml6", str(CLIENTS / "fullscreen-client.qml"), "--", str(signals)], stdout=log, stderr=subprocess.STDOUT)
    wait_for(lambda: TITLE in kwin_titles(), 60)
    activate(TITLE)
    wait_for(lambda: window_state(TITLE) == FULLSCREEN, 60)
    for sample in range(4):
        state = window_state(TITLE)
        print(f"sample {sample}: {state}")
        if state != FULLSCREEN:
            problems.append(f"sample {sample}: window is not fullscreen on the output ({state})")
    activate("A")
    unfocused = [window_state(TITLE) for _ in range(5)]
    print(f"while unfocused: {unfocused}")
    if any(state != FULLSCREEN for state in unfocused):
        problems.append(f"window did not stay fullscreen and still while unfocused ({unfocused})")
    wait_for(lambda: not overlay_problems(), 60)
    problems += overlay_problems()
    (signals / "leave-fullscreen").touch()
    wait_for(lambda: (window_state(TITLE) or "").startswith("false|") and " 1920x1080" not in window_state(TITLE), 60)
    state = window_state(TITLE)
    print(f"after leaving fullscreen: {state}")
    if not state or not state.startswith("false|") or " 1920x1080" in state:
        problems.append(f"window did not return to its column after leaving fullscreen ({state})")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
