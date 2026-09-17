#!/usr/bin/env python3
"""Drive the row with fake touchscreen swipes, pinches, taps and long presses in a nested KWin."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

BREEZE_DECORATION = """
[org.kde.kdecoration2]
library=org.kde.breeze
theme=Breeze
"""


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "touch.py", timeout=180, extra_kwinrc=BREEZE_DECORATION)


if __name__ == "__main__":
    sys.exit(main())
