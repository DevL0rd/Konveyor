#!/usr/bin/env python3
"""Start without config.kdl, with and without a bundled default, and cycle a fixed-size window's width while config.kdl is read-only."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SCRIPT = """
export QT_QPA_PLATFORM=wayland
python3 {runner} {mode} > "$KONVEYOR_REPORT" 2>&1
"""
MARKER = 'binds {\n    Mod+F7 { spawn "true"; }\n}\n'


def main():
    from nested import run_script

    runner = HERE / "runners" / "config_files.py"
    results = [
        run_script(SCRIPT.format(runner=runner, mode="missing-default"), timeout=180, write_config=False, hidden_data=("konveyor",), notifications=True),
        run_script(SCRIPT.format(runner=runner, mode="bundled-default"), timeout=180, write_config=False, notifications=True,
                   files={"data/konveyor/default-config.kdl": MARKER}),
        run_script(SCRIPT.format(runner=runner, mode="force-resizable"), timeout=180, notifications=True),
    ]
    return max(results)


if __name__ == "__main__":
    sys.exit(main())
