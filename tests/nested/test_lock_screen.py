#!/usr/bin/env python3
"""Lock the screen of a nested KWin and check the lock screen stays out of the row and the row is untouched under it."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "lock_screen.py", timeout=180, lockscreen=True)


if __name__ == "__main__":
    sys.exit(main())
