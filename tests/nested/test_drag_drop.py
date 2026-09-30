#!/usr/bin/env python3
"""Drag windows with a real pointer across columns, stacks, tabs, workspaces and two outputs, cancel with Escape and close one mid-drag."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

HINT = """    gaps 16
    insert-hint {
        color "#ff00ff"
    }
"""

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
    from nested import REPO, run_runner

    config = (REPO / "data" / "default-config.kdl").read_text()
    hinted = config.replace("    gaps 16\n", HINT, 1).replace('titlebar-drag "scroll-view"', 'titlebar-drag "move-window"')
    if hinted.count("insert-hint") != 1 or 'titlebar-drag "move-window"' not in hinted:
        print("the default config no longer has the gaps and titlebar-drag lines this test rewrites")
        return 1
    return run_runner(HERE / "runners" / "drag_drop.py", timeout=400, clients=(), output_count=2, config_kdl=hinted + GREEN_ON_SECOND_OUTPUT)


if __name__ == "__main__":
    sys.exit(main())
