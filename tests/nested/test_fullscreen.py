#!/usr/bin/env python3
"""A window that makes itself fullscreen in a nested KWin must stay fullscreen."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_script

    clients, runner = HERE / "harness" / "clients.py", HERE / "runners" / "fullscreen.py"
    script = f'export QT_QPA_PLATFORM=wayland\npython3 {clients} client.qml A > "$KONVEYOR_REPORT" 2>&1 && python3 {runner} > "$KONVEYOR_REPORT" 2>&1\n'
    return run_script(script, timeout=180)


if __name__ == "__main__":
    sys.exit(main())
