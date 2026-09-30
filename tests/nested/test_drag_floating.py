#!/usr/bin/env python3
"""Switch a dragged window between tiled and floating with a right click or a bind, like niri."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from dragging import drag_config
    from nested import run_runner

    return run_runner(HERE / "runners" / "drag_floating.py", config_kdl=drag_config('        color "#ff00ff"\n'), timeout=300)


if __name__ == "__main__":
    sys.exit(main())
