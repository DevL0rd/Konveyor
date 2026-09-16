#!/usr/bin/env python3
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import activate, active_title, konveyor_action, konveyor_windows


def engine_focus():
    return next((window.get("title") for window in konveyor_windows() if window.get("is_focused")), None)


def perform(name):
    konveyor_action(name)
    time.sleep(1.0)


def main():
    problems = []
    steps = [
        ("focus-column-first", lambda: perform("focus-column-first")),
        ("focus-column-right", lambda: perform("focus-column-right")),
        ("focus-column-right", lambda: perform("focus-column-right")),
        ("focus-column-left", lambda: perform("focus-column-left")),
        ("kwin activates A", lambda: (activate("A"), time.sleep(1.0))),
        ("focus-column-last", lambda: perform("focus-column-last")),
        ("close-window", lambda: perform("close-window")),
    ]
    for step, run in steps:
        run()
        engine, kwin = engine_focus(), active_title()
        print(f"{step:20} engine={engine} kwin={kwin}")
        if engine != kwin:
            problems.append(f"{step}: engine focused {engine} but KWin activated {kwin}")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
