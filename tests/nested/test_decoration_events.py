#!/usr/bin/env python3
"""Focus, unfocus and urgent event effects reach the right windows, skip fullscreen monitors and follow windows that close, hide or float."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import NARROW_COLUMNS, run_runner

    return run_runner(HERE / "runners" / "decoration_events.py", timeout=420, extra_config=NARROW_COLUMNS)


if __name__ == "__main__":
    sys.exit(main())
