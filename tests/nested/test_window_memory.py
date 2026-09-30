#!/usr/bin/env python3
"""Start with an unreadable, then a read-only window-memory.json and check window sizes are still remembered or the failure is logged."""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

MEMORY = "state/konveyor/window-memory.json"
QUARTER = json.dumps({"org.qt-project.qml": {"column-width": {"proportion": True, "value": 0.25}}})


def main():
    from nested import REPO, run_runner

    config = (REPO / "data" / "default-config.kdl").read_text().replace("remember-window-sizes false", "remember-window-sizes true")
    runner = HERE / "runners" / "window_memory.py"
    corrupt = run_runner(runner, arguments="corrupt", clients=(), timeout=180, config_kdl=config, files={MEMORY: "{not json"})
    read_only = run_runner(runner, arguments="read-only", clients=(), timeout=180, config_kdl=config, files={MEMORY: QUARTER})
    return max(corrupt, read_only)


if __name__ == "__main__":
    sys.exit(main())
