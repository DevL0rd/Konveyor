#!/usr/bin/env python3
"""Run binds on a layout without their letter, hold them to repeat, and use ISO_Level3_Shift as the Mod key."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

KXKBRC = """[Layout]
LayoutList=us,ru
Options=lv3:ralt_switch
ResetOldOptions=true
Use=true
"""


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "key_binds.py", timeout=300, global_shortcuts=True, clients=("A",), files={"config/kxkbrc": KXKBRC})


if __name__ == "__main__":
    sys.exit(main())
