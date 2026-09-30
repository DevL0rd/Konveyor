#!/usr/bin/env python3
"""Call every method of org.kde.Konveyor, including the error paths, and follow config reloads and LoadConfigFile."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "dbus.py", timeout=300, notifications=True)


if __name__ == "__main__":
    sys.exit(main())
