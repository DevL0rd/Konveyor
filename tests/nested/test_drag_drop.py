#!/usr/bin/env python3
"""Drag windows with a real pointer across columns, stacks, tabs, workspaces and two outputs, scroll the row instead, cancel with Escape and close one mid-drag."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

GREEN_ON_SECOND_OUTPUT = """
window-rule {
    default-column-width { proportion 0.3; }
}

output "Virtual-1" {
    layout {
        insert-hint {
            color "#00ff00"
        }
    }
}
"""


def main():
    from dragging import drag_config
    from nested import run_runner

    config = drag_config('        color "#ff00ff"\n') + GREEN_ON_SECOND_OUTPUT
    return run_runner(HERE / "runners" / "drag_drop.py", config_kdl=config, timeout=400, clients=(), output_count=2)


if __name__ == "__main__":
    sys.exit(main())
