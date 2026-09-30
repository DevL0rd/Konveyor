#!/usr/bin/env python3
"""Rotate the output to portrait and back, then change its scale, and check the row follows the new logical size."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "rotation.py", timeout=180)


if __name__ == "__main__":
    sys.exit(main())
