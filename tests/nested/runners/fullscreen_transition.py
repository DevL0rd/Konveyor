#!/usr/bin/env python3
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import konveyor_action, konveyor_windows, window_state

TITLE = "TransitionPending"
FULLSCREEN = "true|0,0 1920x1080"


def main():
    problems = []
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        if any(window["title"] == TITLE for window in konveyor_windows()):
            break
        time.sleep(0.05)
    else:
        problems.append("client did not begin its fullscreen transition")
    konveyor_action("set-column-width", "+10%")
    time.sleep(4)
    state = window_state(TITLE)
    print(f"after windowed-to-fullscreen transition: {state}")
    if state != FULLSCREEN:
        problems.append(f"fullscreen transition kept windowed geometry ({state})")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
