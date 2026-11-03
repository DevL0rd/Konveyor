#!/usr/bin/env python3
"""A window spilling onto the neighbouring output when no border is drawn, and an output unplugged under the layout."""

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(
        HERE / "runners" / "plugin_outputs.py", timeout=300, clients=(), output_count=2
    )


if __name__ == "__main__":
    sys.exit(main())
