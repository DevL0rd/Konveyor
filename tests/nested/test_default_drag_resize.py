#!/usr/bin/env python3
"""With the shipped config and Breeze title bars, a plain title bar drag moves a tiled window and a border drag resizes its column."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import BREEZE_DECORATION, run_runner

    return run_runner(HERE / "runners" / "default_drag_resize.py", timeout=180, extra_kwinrc=BREEZE_DECORATION)


if __name__ == "__main__":
    sys.exit(main())
