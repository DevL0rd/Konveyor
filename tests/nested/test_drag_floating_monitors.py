#!/usr/bin/env python3
"""A floating window dragged onto another monitor stays there and belongs to that monitor."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from dragging import drag_config
    from nested import run_runner

    return run_runner(HERE / "runners" / "drag_floating_monitors.py", config_kdl=drag_config(), timeout=300, output_count=2, clients=("A", "B"))


if __name__ == "__main__":
    sys.exit(main())
