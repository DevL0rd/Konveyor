#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


SCRIPT = """
export QT_QPA_PLATFORM=wayland
qml6 {client} -- Target 1000 700 &
sleep 2
qml6 {client} -- Cover 700 500 &
sleep 2
qml6 {overlay} -- "Konveyor Monitor Overlay 1 0" 180 32 &
qml6 {overlay} -- "Konveyor Monitor Overlay 1 1" 260 32 &
qml6 {overlay} -- "Konveyor Monitor Overlay 1 2" 220 32 &
qml6 {overlay} -- "Konveyor Monitor Panel 1 1" 500 360 &
sleep 3
python3 {runner} > "$KONVEYOR_REPORT" 2>&1
"""


def main():
    from nested import run_script

    return run_script(
        SCRIPT.format(
            client=HERE / "clients" / "client.qml",
            overlay=HERE / "clients" / "overlay-client.qml",
            runner=HERE / "runners" / "monitor_overlay.py",
        ),
        timeout=90,
    )


if __name__ == "__main__":
    sys.exit(main())
