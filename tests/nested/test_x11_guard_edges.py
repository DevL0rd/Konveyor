#!/usr/bin/env python3
"""Throw odd X11 requests at a fullscreen game: unknown windows, odd message formats, minimize requests while active and in the background, hint changes, experiment switches and unloading."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

EXPERIMENTS = """
experiments {
    prevent-fullscreen-minimize
    prevent-fullscreen-exit
}
"""

SCRIPT = """
export QT_QPA_PLATFORM=wayland
python3 {clients} client.qml A > "$KONVEYOR_REPORT" 2>&1 || exit
python3 {runner} > "$KONVEYOR_REPORT" 2>&1
"""


def main():
    from nested import run_script

    return run_script(SCRIPT.format(clients=HERE / "harness" / "clients.py", runner=HERE / "runners" / "x11_guard_edges.py"),
                      timeout=300, extra_config=EXPERIMENTS, xwayland=True)


if __name__ == "__main__":
    sys.exit(main())
