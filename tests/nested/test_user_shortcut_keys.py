#!/usr/bin/env python3
"""Keep a key the user gave to another component's action when Konveyor loads again."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "user_shortcut_keys.py", clients=(), timeout=240, global_shortcuts=True)


if __name__ == "__main__":
    sys.exit(main())
