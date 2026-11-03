#!/usr/bin/env python3
"""Round one corner at a time in a nested KWin and check that corner of window B is transparent."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

RADII = {"top-right": "0 12 0 0", "bottom-right": "0 0 12 0", "bottom-left": "0 0 0 12"}


def main():
    from nested import run_runner

    results = [
        run_runner(HERE / "runners" / "corners.py", timeout=120, extra_config=f"window-rule {{\n    geometry-corner-radius {radii}\n    clip-to-geometry true\n}}\n", arguments=f"{corner} B")
        for corner, radii in RADII.items()
    ]
    return max(results)


if __name__ == "__main__":
    sys.exit(main())
