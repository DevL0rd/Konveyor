#!/usr/bin/env python3
"""Tools the effect starts: one that is missing, one that runs, an overlay tool that cannot start, and the hotkey overlay action."""

import os
import shutil
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    fake = tempfile.mkdtemp(prefix="konveyor-tools-")
    path = [
        entry for entry in os.environ["PATH"].split(":") if "/.local/bin" not in entry
    ]
    os.environ["PATH"] = ":".join([fake, *path])
    os.environ["KONVEYOR_FAKE_BIN"] = fake
    try:
        return run_runner(
            HERE / "runners" / "plugin_tools.py", timeout=240, notifications=True
        )
    finally:
        shutil.rmtree(fake, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
