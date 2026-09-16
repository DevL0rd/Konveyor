#!/usr/bin/env python3
"""Move focus in a nested KWin and check KWin activates what the layout focuses."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "focus.py", timeout=120)


if __name__ == "__main__":
    sys.exit(main())
