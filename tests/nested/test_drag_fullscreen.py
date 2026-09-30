#!/usr/bin/env python3
"""Make a window fullscreen, from the app and from a bind, while it is being dragged."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from dragging import drag_config
    from nested import run_runner

    return run_runner(HERE / "runners" / "drag_fullscreen.py", config_kdl=drag_config(), timeout=300)


if __name__ == "__main__":
    sys.exit(main())
