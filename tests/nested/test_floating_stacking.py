#!/usr/bin/env python3
"""Keep floating windows above tiled ones whatever raises a window, restack only within one screen, and leave focus on an unmanaged overlay."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "floating_stacking.py", timeout=400, output_count=2)


if __name__ == "__main__":
    sys.exit(main())
