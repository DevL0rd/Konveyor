#!/usr/bin/env python3
"""Drag windows with a real pointer and check the drop hint and each of its settings in screenshots."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

ROUNDED_C = """
window-rule {
    match title="^C$"
    geometry-corner-radius 40
}
"""


def main():
    from dragging import drag_config
    from nested import run_runner

    config = drag_config('        color "#ff00ff"\n') + ROUNDED_C
    return run_runner(HERE / "runners" / "drop_hint.py", config_kdl=config, timeout=300,
                      files={"config/kdeglobals": "[Colors:Selection]\nBackgroundNormal=0,255,0\n"})


if __name__ == "__main__":
    sys.exit(main())
