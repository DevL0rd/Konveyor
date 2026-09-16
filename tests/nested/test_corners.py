#!/usr/bin/env python3
"""Clip windows to rounded corners in a nested KWin and check their corners are transparent."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

RULE = """
window-rule {
    geometry-corner-radius 12
    clip-to-geometry true
}
"""


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "corners.py", timeout=120, extra_config=RULE)


if __name__ == "__main__":
    sys.exit(main())
