#!/usr/bin/env python3
"""Move keyboard focus with the pointer elsewhere in each warp-mouse-to-focus mode, and hover windows with focus-follows-mouse."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "pointer_focus.py", timeout=240)


if __name__ == "__main__":
    sys.exit(main())
