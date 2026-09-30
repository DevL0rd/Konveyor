#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_script

    script = f'export QT_QPA_PLATFORM=wayland\npython3 {HERE / "runners" / "multimonitor_focus.py"} > "$KONVEYOR_REPORT" 2>&1\n'
    return run_script(script, timeout=240, output_count=2)


if __name__ == "__main__":
    sys.exit(main())
