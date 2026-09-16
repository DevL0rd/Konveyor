#!/usr/bin/env python3
"""A window that makes itself fullscreen in a nested KWin must stay fullscreen."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SCRIPT = """
export QT_QPA_PLATFORM=wayland
qml6 {client} -- A 700 500 &
sleep 2
qml6 {fullscreen} &
python3 {runner} > "$KONVEYOR_REPORT" 2>&1
"""


def main():
    from nested import run_script

    return run_script(SCRIPT.format(client=HERE / "clients" / "client.qml",
                                    fullscreen=HERE / "clients" / "fullscreen-client.qml",
                                    runner=HERE / "runners" / "fullscreen.py"), timeout=120)


if __name__ == "__main__":
    sys.exit(main())
