#!/usr/bin/env python3
"""Swipe and keyboard between workspaces with KWin's Slide enabled to check that a
gesture switch is not replayed by Slide and that the focus ring hands off per mode."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SLOW_SLIDE = """
konveyor_test_touchpadEnabled=true
slideEnabled=true

[KDE]
AnimationDurationFactor=3
"""


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "workspace_transition.py", timeout=420, clients=(), extra_kwinrc=SLOW_SLIDE,
                      force_animations=True)


if __name__ == "__main__":
    sys.exit(main())
