#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return max(run_runner(HERE / "runners" / "fullscreen_restore_x11.py", timeout=240, xwayland=True, width=width, height=height,
                      extra_config="experiments { prevent-fullscreen-exit false; prevent-fullscreen-minimize false; }")
               for width, height in ((1920, 1080), (5120, 1440)))


if __name__ == "__main__":
    sys.exit(main())
