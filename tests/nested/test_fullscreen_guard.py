#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SCRIPT = """
export QT_QPA_PLATFORM=wayland
python3 {clients} client.qml A > "$KONVEYOR_REPORT" 2>&1 && python3 {runner} {mode} > "$KONVEYOR_REPORT" 2>&1
"""

EXPERIMENTS = """
experiments {
    prevent-fullscreen-minimize
    prevent-fullscreen-exit
}
"""


def main():
    from nested import run_script

    results = [run_script(SCRIPT.format(clients=HERE / "harness" / "clients.py", runner=HERE / "runners" / "fullscreen_guard.py", mode=mode),
                          timeout=240, extra_config=EXPERIMENTS, xwayland=True) for mode in ("", "reload")]
    return max(results)


if __name__ == "__main__":
    sys.exit(main())
