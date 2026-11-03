#!/usr/bin/env python3
"""Animated borders and event effects hold still on battery power and with reduced motion, follow a fake UPower coming and going, and stay out of fullscreen monitors."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import NARROW_COLUMNS, run_runner

    return run_runner(HERE / "runners" / "decoration_stillness.py", timeout=420, extra_config=NARROW_COLUMNS, system_bus=True)


if __name__ == "__main__":
    sys.exit(main())
