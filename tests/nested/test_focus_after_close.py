#!/usr/bin/env python3
"""Close the focused last column while KWin's previously active window is still on screen and check the layout, not KWin, picks the next focus."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import NARROW_COLUMNS, run_runner

    return run_runner(HERE / "runners" / "focus.py", timeout=120, extra_config=NARROW_COLUMNS)


if __name__ == "__main__":
    sys.exit(main())
