#!/usr/bin/env python3
"""Start without config.kdl, with and without a bundled default, and cycle a fixed-size window's width while config.kdl is read-only."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

MARKER = 'binds {\n    Mod+F7 { spawn "true"; }\n}\n'


def main():
    from nested import run_runner

    runner = HERE / "runners" / "config_files.py"
    results = [
        run_runner(runner, arguments="missing-default", clients=(), timeout=180, write_config=False, hidden_data=("konveyor",), notifications=True),
        run_runner(runner, arguments="bundled-default", clients=(), timeout=180, write_config=False, notifications=True,
                   files={"data/konveyor/default-config.kdl": MARKER}),
        run_runner(runner, arguments="force-resizable", clients=(), timeout=180, notifications=True),
    ]
    return max(results)


if __name__ == "__main__":
    sys.exit(main())
