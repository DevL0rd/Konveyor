#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

X11_SCRIPT = """
export QT_QPA_PLATFORM=wayland
python3 {clients} client.qml A > "$KONVEYOR_REPORT" 2>&1 || exit
python3 {game} X11Game --take-focus &
python3 {runner} X11Game > "$KONVEYOR_REPORT" 2>&1
"""

WAYLAND_SCRIPT = """
export QT_QPA_PLATFORM=wayland
python3 {clients} client.qml A G > "$KONVEYOR_REPORT" 2>&1 || exit
python3 {runner} G --make-fullscreen > "$KONVEYOR_REPORT" 2>&1
"""


def main():
    from nested import run_script

    paths = {"clients": HERE / "harness" / "clients.py", "game": HERE / "clients" / "x11-game.py",
             "runner": HERE / "runners" / "fullscreen_workspaces.py"}
    x11 = run_script(X11_SCRIPT.format(**paths), timeout=300, xwayland=True)
    wayland = run_script(WAYLAND_SCRIPT.format(**paths), timeout=300)
    return max(x11, wayland)


if __name__ == "__main__":
    sys.exit(main())
