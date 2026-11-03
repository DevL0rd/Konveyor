#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "fullscreen_locked_size.py", timeout=150, clients=("A",), arguments=" ".join(sys.argv[1:]))


if __name__ == "__main__":
    sys.exit(main())
