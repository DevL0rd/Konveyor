#!/usr/bin/env python3
"""Styled rings and borders blend, fall back to the accent colour, restyle on reload, split into ring and border parts, follow urgent windows and survive a window closing."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import NARROW_COLUMNS, run_runner

    return run_runner(HERE / "runners" / "decoration_styles.py", timeout=300, extra_config=NARROW_COLUMNS,
                      files={"config/kdeglobals": "[Colors:Selection]\nBackgroundNormal=255,0,0\n"})


if __name__ == "__main__":
    sys.exit(main())
