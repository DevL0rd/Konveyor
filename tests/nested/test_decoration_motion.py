#!/usr/bin/env python3
"""Light fronts travel between the windows that gained and lost focus and out from a click, and while-mode effects last as long as the input keeps coming."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import NARROW_COLUMNS, run_runner

    return run_runner(HERE / "runners" / "decoration_motion.py", timeout=420, extra_config=NARROW_COLUMNS)


if __name__ == "__main__":
    sys.exit(main())
