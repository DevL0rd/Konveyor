#!/usr/bin/env python3
"""Screenshot tabbed columns: the shown tab follows focus."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "tabs.py", timeout=420, client="colored-client.qml")


if __name__ == "__main__":
    sys.exit(main())
