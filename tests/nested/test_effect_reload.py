#!/usr/bin/env python3
"""Reload the effect while windows are open, like an update does, and check the windows come through it unchanged."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "effect_reload.py", timeout=300, xwayland=True)


if __name__ == "__main__":
    sys.exit(main())
