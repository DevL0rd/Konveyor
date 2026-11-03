#!/usr/bin/env python3
"""Drive the effect's less common paths: a missing Overview effect, app rule queries for gone windows, fixed-size windows cycling their width, maximize requests and windows on every desktop."""

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(
        HERE / "runners" / "plugin_edges.py", timeout=300, notifications=True
    )


if __name__ == "__main__":
    sys.exit(main())
