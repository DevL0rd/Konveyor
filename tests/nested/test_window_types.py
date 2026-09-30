#!/usr/bin/env python3
"""Open Wayland and X11 windows of every type and check which ones Konveyor tiles, including windows pinned to all desktops."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

STICKY_RULE = """[General]
count=1
rules=sticky

[sticky]
Description=X-Sticky starts on all desktops
title=X-Sticky
titlematch=1
desktops=
desktopsrule=3
"""


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "window_types.py", clients=(), timeout=240, xwayland=True,
                      files={"config/kwinrulesrc": STICKY_RULE})


if __name__ == "__main__":
    sys.exit(main())
