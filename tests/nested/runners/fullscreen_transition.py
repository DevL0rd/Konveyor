#!/usr/bin/env python3
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import konveyor_action, konveyor_windows, wait_for, window_state

TITLE = "TransitionPending"
FULLSCREEN = "true|0,0 1920x1080"


def main():
    problems = []
    if not wait_for(lambda: any(window["title"] == TITLE for window in konveyor_windows()), 60):
        problems.append("client did not begin its fullscreen transition")
    konveyor_action("set-column-width", "+10%")
    Path(os.environ["KONVEYOR_TRANSITION_RELEASE"]).write_text("release\n")
    wait_for(lambda: window_state(TITLE) == FULLSCREEN, 60)
    state = window_state(TITLE)
    print(f"after windowed-to-fullscreen transition: {state}")
    if state != FULLSCREEN:
        problems.append(f"fullscreen transition kept windowed geometry ({state})")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
