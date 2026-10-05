#!/usr/bin/env python3
"""Run plasmashell inside the nested session and check panel filling, hidden desktop widgets and unloading while a panel is filled."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import PLASMASHELL, run_runner

    return run_runner(HERE / "runners" / "plasma.py", timeout=420, clients=("A", "B"), setup=PLASMASHELL)


if __name__ == "__main__":
    sys.exit(main())
