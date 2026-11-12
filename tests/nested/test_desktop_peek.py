#!/usr/bin/env python3
"""Hold Meta with the peek-desktop experiment on: a tap still fires a modifier-only shortcut, a hold fades the other windows, the focused one slides aside once and comes back, and Meta shortcuts, clicks and fullscreen windows are left alone."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "desktop_peek.py", timeout=300, global_shortcuts=True,
                      extra_config="experiments { peek-desktop { on; delay-ms 300; }; }\n")


if __name__ == "__main__":
    sys.exit(main())
