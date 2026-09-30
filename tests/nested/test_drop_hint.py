#!/usr/bin/env python3
"""Drag windows with a real pointer and check the drop hint in screenshots."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

HINT = """    gaps 16
    insert-hint {
        color "#ff00ff"
    }
"""


def main():
    from nested import REPO, run_runner

    config = (REPO / "data" / "default-config.kdl").read_text()
    hinted = config.replace("    gaps 16\n", HINT, 1).replace('titlebar-drag "scroll-view"', 'titlebar-drag "move-window"')
    if hinted.count("insert-hint") != 1 or 'titlebar-drag "move-window"' not in hinted:
        print("the default config no longer has the gaps and titlebar-drag lines this test rewrites")
        return 1
    return run_runner(HERE / "runners" / "drop_hint.py", timeout=300, config_kdl=hinted)


if __name__ == "__main__":
    sys.exit(main())
