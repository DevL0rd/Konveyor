#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import activate, active_title, konveyor_action, konveyor_windows, wait_for


def engine_focus():
    return next((window.get("title") for window in konveyor_windows() if window.get("is_focused")), None)


def main():
    problems = []
    steps = [
        ("focus-column-first", lambda: konveyor_action("focus-column-first"), "A"),
        ("focus-column-right", lambda: konveyor_action("focus-column-right"), "B"),
        ("focus-column-right", lambda: konveyor_action("focus-column-right"), "C"),
        ("focus-column-left", lambda: konveyor_action("focus-column-left"), "B"),
        ("kwin activates A", lambda: activate("A"), "A"),
        ("focus-column-last", lambda: konveyor_action("focus-column-last"), "C"),
        ("close-window", lambda: konveyor_action("close-window"), "A"),
    ]
    for step, run, expected in steps:
        run()
        wait_for(lambda: engine_focus() == active_title() == expected, 60)
        engine, kwin = engine_focus(), active_title()
        print(f"{step:20} engine={engine} kwin={kwin}")
        if engine != kwin:
            problems.append(f"{step}: engine focused {engine} but KWin activated {kwin}")
        elif engine != expected:
            problems.append(f"{step}: expected {expected} focused, got {engine}")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
