#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

RULE = """
window-rule {
    match app-id=r#"^konveyor-strict-forced"#
    force-resizable true
}
"""


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "strict_size.py", timeout=400, clients=("A",), extra_config=RULE, xwayland=True)


if __name__ == "__main__":
    sys.exit(main())
