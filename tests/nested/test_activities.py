#!/usr/bin/env python3
"""Move windows between KDE activities and switch activities, and check only the current activity's windows take room in the row."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "activities.py", timeout=240, clients=("A", "B", "C", "D"))


if __name__ == "__main__":
    sys.exit(main())
