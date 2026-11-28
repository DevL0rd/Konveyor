#!/usr/bin/env python3
"""With Konveyor's overview enabled, toggle-overview paints every workspace at once
instead of handing the desktop to KDE, the wheel pans the stack and a window can be
dragged onto another workspace's card."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "overview.py", timeout=420, clients=(), extra_config="overview { enabled true; }\n",
                      extra_kwinrc="konveyor_test_touchpadEnabled=true")


if __name__ == "__main__":
    sys.exit(main())
