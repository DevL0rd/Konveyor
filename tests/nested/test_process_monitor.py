#!/usr/bin/env python3
"""Load the Process Monitor frame telemetry effect next to Konveyor and check Frames, Watch and its Frame signal."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SCRIPT = """
export QT_QPA_PLATFORM=wayland
python3 {runner} > "$KONVEYOR_REPORT" 2>&1
"""


def main():
    from nested import run_script

    return run_script(SCRIPT.format(runner=HERE / "runners" / "process_monitor.py"), timeout=180,
                      extra_kwinrc="process_monitor_telemetryEnabled=true")


if __name__ == "__main__":
    sys.exit(main())
