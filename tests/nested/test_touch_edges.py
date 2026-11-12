#!/usr/bin/env python3
"""Touch on a title bar: a tap, a jitter before the drag, a second finger already down and the window closing under the finger."""

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import BREEZE_DECORATION, run_runner

    return run_runner(
        HERE / "runners" / "touch_edges.py",
        timeout=180,
        extra_kwinrc=BREEZE_DECORATION,
    )


if __name__ == "__main__":
    sys.exit(main())
