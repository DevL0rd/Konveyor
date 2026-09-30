#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

HALF_WIDTH_COLUMNS = """
window-rule {
    default-column-width { proportion 0.5; }
}
"""


def main():
    from nested import run_script

    report = '> "$KONVEYOR_REPORT" 2>&1'
    return run_script(f'export QT_QPA_PLATFORM=wayland\npython3 {HERE / "runners" / "multimonitor.py"} {report}\n', timeout=240,
                      extra_config=HALF_WIDTH_COLUMNS, output_count=2)


if __name__ == "__main__":
    sys.exit(main())
