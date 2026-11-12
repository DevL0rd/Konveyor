#!/usr/bin/env python3
"""Wrap a row of wide centered columns in a nested KWin and check the first column follows the last one only with wrap-columns."""
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

WIDE = """
window-rule {
    default-column-width { proportion 0.9; }
}
"""


def main():
    from nested import REPO, run_runner

    config = (REPO / "data" / "default-config.kdl").read_text()
    if 'center-focused-column "never"' not in config or "wrap-columns false" not in config:
        print("the default config no longer has the layout lines this test rewrites")
        return 1
    config = config.replace('center-focused-column "never"', 'center-focused-column "always"') + WIDE
    return run_runner(HERE / "runners" / "column_wrap.py", timeout=180, config_kdl=config, clients=("A", "B", "C", "D"),
                      arguments=os.environ.get("KONVEYOR_SHOTS_DIR", ""))


if __name__ == "__main__":
    sys.exit(main())
