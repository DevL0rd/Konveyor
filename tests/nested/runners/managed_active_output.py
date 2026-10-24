#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from fakepointer import move
from kwinsession import active_title, open_client, wait_for
from multimonitor_focus import active_output, engine_focus, engine_output, outputs, placed


def main():
    screens = outputs()
    names = sorted(screens, key=lambda name: screens[name][0])
    left, right = names[:2]
    move(screens[left][0] + 400, 540, settle=False)
    open_client("managed-output")
    if not wait_for(lambda: placed("managed-output") and active_title() == "managed-output", 60):
        print("client did not become the focused managed window")
        return 1

    move(screens[right][0] + 400, 540, settle=False)
    if not wait_for(lambda: active_output() == right, 10):
        print(f"KWin active output did not move to {right}")
        return 1

    actual_focus = engine_focus()
    actual_output = engine_output()
    print(f"pointer on {right}; KWin active={active_title()} engine focus={actual_focus} engine output={actual_output}")
    if active_title() != "managed-output" or actual_focus != "managed-output" or actual_output != left:
        print("RESULT: FAIL")
        return 1

    print("RESULT: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
