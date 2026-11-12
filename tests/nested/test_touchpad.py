#!/usr/bin/env python3
"""Drive the row with swipes, pinches and clicks from a virtual touchpad added to a nested KWin."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner, unwrapped_config

    return run_runner(HERE / "runners" / "touchpad.py", timeout=300, config_kdl=unwrapped_config(), global_shortcuts=True, extra_kwinrc="konveyor_test_touchpadEnabled=true")


if __name__ == "__main__":
    sys.exit(main())
