#!/usr/bin/env python3
"""Open a fullscreen window tagged region-editor, as Spectacle does for rectangular screen recording, and check it is never tiled."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "tagged_overlay.py", clients=(), timeout=240)


if __name__ == "__main__":
    sys.exit(main())
