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
    from nested import run_runner

    return run_runner(HERE / "runners" / "multimonitor.py", timeout=180, extra_config=HALF_WIDTH_COLUMNS, output_count=2)


if __name__ == "__main__":
    sys.exit(main())
