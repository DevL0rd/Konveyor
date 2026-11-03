#!/usr/bin/env python3
"""Event effects stay on the output and workspace of the window that played them and skip outputs covered by a fullscreen window."""

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import NARROW_COLUMNS, run_runner

    return run_runner(
        HERE / "runners" / "decoration_places.py",
        timeout=420,
        clients=(),
        output_count=2,
        extra_config=NARROW_COLUMNS,
    )


if __name__ == "__main__":
    sys.exit(main())
