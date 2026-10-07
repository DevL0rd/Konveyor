#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

ANY_MONITOR = 'workspace "web"\n'


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "named_workspaces.py", timeout=240, extra_config=ANY_MONITOR, clients=(), output_count=2)


if __name__ == "__main__":
    sys.exit(main())
