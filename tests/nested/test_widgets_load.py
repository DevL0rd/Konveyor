#!/usr/bin/env python3
"""Install every Konveyor widget into the nested session's home, add each one to a desktop in plasmashell and check that all of them load."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_in_plasmashell

    return run_in_plasmashell(HERE / "runners" / "widgets_load.py", timeout=420, clients=())


if __name__ == "__main__":
    sys.exit(main())
