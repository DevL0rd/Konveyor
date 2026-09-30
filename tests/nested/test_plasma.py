#!/usr/bin/env python3
"""Run plasmashell inside the nested session and check panel filling, hidden desktop widgets and unloading while a panel is filled."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

PLASMASHELL = """
/usr/lib/kactivitymanagerd > "$KONVEYOR_TEST_ROOT/kactivitymanagerd.log" 2>&1 &
for _ in $(seq 600); do qdbus6 org.kde.ActivityManager > /dev/null 2>&1 && break; sleep 0.1; done
plasmashell --no-respawn > "$KONVEYOR_TEST_ROOT/plasmashell.log" 2>&1 &
"""


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "plasma.py", timeout=420, clients=("A", "B"), setup=PLASMASHELL)


if __name__ == "__main__":
    sys.exit(main())
