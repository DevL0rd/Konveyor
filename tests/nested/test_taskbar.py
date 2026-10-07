#!/usr/bin/env python3
"""Put the Konveyor Taskbar in a plasmashell panel and check that its icons follow the column order both ways."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import PLASMASHELL, run_runner

    return run_runner(HERE / "runners" / "taskbar.py", timeout=420, setup=PLASMASHELL)


if __name__ == "__main__":
    sys.exit(main())
