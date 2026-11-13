#!/usr/bin/env python3
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    # CI runs the QML clients through the software Qt Quick backend, but KWin's
    # overview draws its window thumbnails through its own scene graph and
    # leaves them out in that backend, so let this session pick its real one.
    os.environ.pop("QT_QUICK_BACKEND", None)
    return run_runner(HERE / "runners" / "inactive_desktops.py", timeout=360, clients=(), global_shortcuts=True,
                      extra_kwinrc="konveyor_test_touchpadEnabled=true")


if __name__ == "__main__":
    sys.exit(main())
