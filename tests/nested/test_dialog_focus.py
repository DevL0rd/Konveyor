#!/usr/bin/env python3
"""Hover and click dialogs and floated windows over tiled windows with focus-follows-mouse, slowly and quickly."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "dialog_focus.py", timeout=600, xwayland=True)


if __name__ == "__main__":
    sys.exit(main())
