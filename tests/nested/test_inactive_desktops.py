#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "inactive_desktops.py", timeout=240, clients=())


if __name__ == "__main__":
    sys.exit(main())
