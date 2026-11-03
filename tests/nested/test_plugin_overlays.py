#!/usr/bin/env python3
"""Windows named like monitor overlays that attach to themselves, that already belong to their target and that belong to another window."""

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "plugin_overlays.py", timeout=300)


if __name__ == "__main__":
    sys.exit(main())
