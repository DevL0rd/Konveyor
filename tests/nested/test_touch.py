#!/usr/bin/env python3
"""Drive the row with fake touchscreen swipes, pinches, taps and long presses in a nested KWin."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import BREEZE_DECORATION, REPO, run_runner

    config = (REPO / "data" / "default-config.kdl").read_text()
    scrolling = config.replace('titlebar-drag "move-window"', 'titlebar-drag "scroll-view"')
    if scrolling == config:
        print("the default config no longer has the titlebar-drag line this test rewrites")
        return 1
    return run_runner(HERE / "runners" / "touch.py", timeout=180, config_kdl=scrolling, extra_kwinrc=BREEZE_DECORATION)


if __name__ == "__main__":
    sys.exit(main())
