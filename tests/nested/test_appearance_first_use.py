#!/usr/bin/env python3
"""Pick the first starting point in Appearance and see the settings write it as the focus ring style and the focused window's frame start moving."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "appearance_first_use.py", timeout=240, clients=("A", "B", "C"))


if __name__ == "__main__":
    sys.exit(main())
