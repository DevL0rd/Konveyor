#!/usr/bin/env python3
"""Open the on-screen keyboard over a two-row column in a nested KWin and check the column moves above it and still takes typing."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

KEYBOARD = """
[Wayland]
VirtualKeyboardEnabled=true
VirtualKeyboardMode=2
"""


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "osk.py", timeout=150, client="text-client.qml", extra_kwinrc=KEYBOARD, input_method="/usr/bin/plasma-keyboard")


if __name__ == "__main__":
    sys.exit(main())
