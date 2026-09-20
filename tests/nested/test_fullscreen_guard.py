#!/usr/bin/env python3
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SCRIPT = """
export QT_QPA_PLATFORM=wayland
qml6 {client} -- A 700 500 &
sleep 1
{guard_client} GuardUnfullscreen unfullscreen &
sleep 1
{guard_client} GuardMinimize minimize &
sleep 1
python3 {x11_guard} X11GuardUnfullscreen unfullscreen &
sleep 2
python3 {x11_guard} X11GuardMinimize minimize &
sleep 2
python3 {runner} > "$KONVEYOR_REPORT" 2>&1
"""

EXPERIMENTS = """
experiments {
    prevent-fullscreen-minimize
    prevent-fullscreen-exit
}
"""


def main():
    from nested import run_script

    guard_client = Path(os.environ.get("KONVEYOR_BUILD_DIR", HERE.parents[1] / "build")) / "bin" / "fullscreen_guard_client"
    return run_script(SCRIPT.format(client=HERE / "clients" / "client.qml",
                                    guard_client=guard_client,
                                    x11_guard=HERE / "clients" / "fullscreen-guard-x11-client.py",
                                    runner=HERE / "runners" / "fullscreen_guard.py"),
                      timeout=60, extra_config=EXPERIMENTS, xwayland=True)


if __name__ == "__main__":
    sys.exit(main())
