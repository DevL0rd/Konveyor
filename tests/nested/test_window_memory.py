#!/usr/bin/env python3
"""Start with an unreadable, then a read-only window-memory.json and check window sizes are still remembered or the failure is logged."""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SCRIPT = """
export QT_QPA_PLATFORM=wayland
python3 {runner} {mode} > "$KONVEYOR_REPORT" 2>&1
"""
MEMORY = "state/konveyor/window-memory.json"
QUARTER = json.dumps({"org.qt-project.qml": {"column-width": {"proportion": True, "value": 0.25}}})


def main():
    from nested import REPO, run_script

    config = (REPO / "data" / "default-config.kdl").read_text().replace("remember-window-sizes false", "remember-window-sizes true")
    runner = HERE / "runners" / "window_memory.py"
    corrupt = run_script(SCRIPT.format(runner=runner, mode="corrupt"), timeout=180, config_kdl=config, files={MEMORY: "{not json"})
    read_only = run_script(SCRIPT.format(runner=runner, mode="read-only"), timeout=180, config_kdl=config, files={MEMORY: QUARTER})
    return max(corrupt, read_only)


if __name__ == "__main__":
    sys.exit(main())
