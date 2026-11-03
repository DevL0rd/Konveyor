#!/usr/bin/env python3
"""Horizontal wheel turns point sideways, and a locked screen counts no clicks, wheel turns or keys for the windows behind it."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "input_locked.py", timeout=240, clients=(), lockscreen=True)


if __name__ == "__main__":
    sys.exit(main())
