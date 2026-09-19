#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SCRIPT = """
export QT_QPA_PLATFORM=wayland
qml6 {client} &
python3 {runner} > "$KONVEYOR_REPORT" 2>&1
"""


def main():
    from nested import run_script

    return run_script(SCRIPT.format(client=HERE / "clients" / "transition-fullscreen-client.qml",
                                    runner=HERE / "runners" / "fullscreen_transition.py"), timeout=60)


if __name__ == "__main__":
    sys.exit(main())
