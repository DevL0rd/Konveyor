#!/usr/bin/env python3
"""Set every corner radius to 0 at runtime on Breeze decorated windows and check the focus ring corner turns square."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import BREEZE_DECORATION, run_runner

    return run_runner(HERE / "runners" / "corners_square.py", timeout=180, extra_kwinrc=BREEZE_DECORATION)


if __name__ == "__main__":
    sys.exit(main())
