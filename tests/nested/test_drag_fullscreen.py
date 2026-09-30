#!/usr/bin/env python3
"""Make a window fullscreen, from the app and from a bind, while it is being dragged."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import REPO, run_runner

    config = (REPO / "data" / "default-config.kdl").read_text()
    moving = config.replace('titlebar-drag "scroll-view"', 'titlebar-drag "move-window"')
    if moving == config:
        print("the default config no longer has the titlebar-drag line this test rewrites")
        return 1
    return run_runner(HERE / "runners" / "drag_fullscreen.py", timeout=300, config_kdl=moving)


if __name__ == "__main__":
    sys.exit(main())
