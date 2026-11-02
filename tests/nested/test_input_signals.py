#!/usr/bin/env python3
"""Clicks, wheel and keys move the per-window input signals on the right window, apps still get every event, and nothing is observed unless a style or event effect asks for it."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "input_signals.py", timeout=240, clients=())


if __name__ == "__main__":
    sys.exit(main())
