#!/usr/bin/env python3
"""An X11 fullscreen game that keeps asking for fullscreen must stay in place without flickering while other columns are brought over it."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SCRIPT = """
export QT_QPA_PLATFORM=wayland
python3 {clients} client.qml A > "$KONVEYOR_REPORT" 2>&1 || exit
python3 {game} X11Game &
python3 {runner} > "$KONVEYOR_REPORT" 2>&1
"""


def main():
    from nested import run_script

    return run_script(SCRIPT.format(clients=HERE / "harness" / "clients.py", game=HERE / "clients" / "x11-game.py",
                                    runner=HERE / "runners" / "fullscreen_x11.py"), timeout=240, xwayland=True)


if __name__ == "__main__":
    sys.exit(main())
