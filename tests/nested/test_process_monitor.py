#!/usr/bin/env python3
"""Load the Process Monitor frame telemetry effect next to Konveyor and check Frames, Watch and its Frame signal."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "process_monitor.py", clients=(), timeout=180,
                      extra_kwinrc="process_monitor_telemetryEnabled=true")


if __name__ == "__main__":
    sys.exit(main())
