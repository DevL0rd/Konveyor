#!/usr/bin/env python3
"""Scrolling the row publishes the view signals only to borders and effects that use them, and settles back to still."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "decoration_views.py", timeout=420)


if __name__ == "__main__":
    sys.exit(main())
