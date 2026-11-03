#!/usr/bin/env python3
"""A window parked off every monitor stays there when its client answers a configure, so a mixed-scale setup never loops on scale changes, and KWin confines it normally again once it is back in view."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "parked_output.py", timeout=300, clients=(), output_count=2)


if __name__ == "__main__":
    sys.exit(main())
