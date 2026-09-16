#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

NARROW_COLUMNS = """
window-rule {
    default-column-width { proportion 0.3; }
}
"""


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "minimize.py", timeout=120, extra_config=NARROW_COLUMNS)


if __name__ == "__main__":
    sys.exit(main())
