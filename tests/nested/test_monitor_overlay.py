#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_script

    runner = HERE / "runners" / "monitor_overlay.py"
    return run_script(f'export QT_QPA_PLATFORM=wayland\npython3 {runner} > "$KONVEYOR_REPORT" 2>&1\n', timeout=300)


if __name__ == "__main__":
    sys.exit(main())
