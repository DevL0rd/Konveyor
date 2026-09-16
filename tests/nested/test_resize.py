#!/usr/bin/env python3
"""Resize windows in a nested KWin and check the row never breaks.

Runs headless, so it never touches the running session. After each width the
focused column must stay fully on screen and the row must not leave dead space
on the right while windows are cut off on the left.
"""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "resize.py", timeout=420)


if __name__ == "__main__":
    sys.exit(main())
