#!/usr/bin/env python3
"""Arrange three outputs in an L and scroll each row, checking windows scrolled out of view never show up on another output."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "parking.py", timeout=420, clients=(), output_count=3)


if __name__ == "__main__":
    sys.exit(main())
