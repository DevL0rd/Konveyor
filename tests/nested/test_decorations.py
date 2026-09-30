#!/usr/bin/env python3
"""Screenshot the focus ring and tab indicator, recolour the ring by changing the KDE accent colour live, and draw gradient borders relative to each window and to the workspace view."""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

DECORATIONS = """focus-ring {
        width 8
        active-color "accent"
        inactive-color "#00ff00"
    }

    tab-indicator {
        width 6
        gap 4
        active-color "#ff00ff"
        inactive-color "#00ffff"
    }"""


def main():
    from nested import REPO, run_runner

    config = (REPO / "data" / "default-config.kdl").read_text()
    decorated = re.sub(r"focus-ring \{[^}]*\}", DECORATIONS, config, count=1)
    if decorated == config:
        print("the default config no longer has the focus-ring block this test rewrites")
        return 1
    return run_runner(HERE / "runners" / "decorations.py", timeout=240, config_kdl=decorated,
                      files={"config/kdeglobals": "[Colors:Selection]\nBackgroundNormal=255,0,0\n"})


if __name__ == "__main__":
    sys.exit(main())
