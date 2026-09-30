#!/usr/bin/env python3
"""Fire every mouse, wheel and touchpad bind and check which of Konveyor and KWin gets each one."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "pointer_binds.py", timeout=500, global_shortcuts=True,
                      extra_kwinrc="konveyor_test_touchpadEnabled=true")


if __name__ == "__main__":
    sys.exit(main())
