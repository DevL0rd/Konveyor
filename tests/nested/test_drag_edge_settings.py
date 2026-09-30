#!/usr/bin/env python3
"""Hold a dragged window at the screen edge with each dnd-edge-view-scroll setting changed and measure how the row scrolls."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from dragging import drag_config
    from nested import run_runner

    return run_runner(HERE / "runners" / "drag_edge_settings.py", config_kdl=drag_config(), timeout=300, clients=("A", "B", "C", "D"))


if __name__ == "__main__":
    sys.exit(main())
