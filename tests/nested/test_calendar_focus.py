#!/usr/bin/env python3
"""Open the real plasmashell clock calendar with focus-follows-mouse on and keep it open while the pointer crosses a tiled window."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_in_plasmashell

    return run_in_plasmashell(HERE / "runners" / "calendar_focus.py", timeout=300, xwayland=True)


if __name__ == "__main__":
    sys.exit(main())
