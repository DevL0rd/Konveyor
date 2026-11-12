#!/usr/bin/env python3
"""Run workspace hotkeys and wheel binds on the output under the pointer, and window hotkeys on the focused window's output unless focus-keys-follow-mouse is on."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "pointer_output_binds.py", clients=(), timeout=400, global_shortcuts=True, output_count=2)


if __name__ == "__main__":
    sys.exit(main())
