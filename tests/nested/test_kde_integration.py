#!/usr/bin/env python3
"""Follow KDE's virtual desktops and the minimize rule as the layout and config change, then unload the effect."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

USER_RULES = """[General]
count=1
rules=user-rule

[user-rule]
Description=A rule the user made
title=keep me
titlematch=1
"""

DESKTOPS = """
[Desktops]
Number=1
Rows=1
"""


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "kde_integration.py", timeout=300, extra_kwinrc=DESKTOPS, files={"config/kwinrulesrc": USER_RULES})


if __name__ == "__main__":
    sys.exit(main())
