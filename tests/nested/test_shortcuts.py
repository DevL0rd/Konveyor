#!/usr/bin/env python3
"""Register every bind with KDE's global shortcuts, press each one, and follow takeovers through reloads and unloading."""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SCRIPT = """
export QT_QPA_PLATFORM=wayland
python3 {runner} > "$KONVEYOR_REPORT" 2>&1
"""


def main():
    from nested import REPO, run_script

    config = (REPO / "data" / "default-config.kdl").read_text()
    harmless = re.sub(r"\{ spawn [^;]*; \}", '{ spawn "true"; }', config).replace("{ show-hotkey-overlay; }", '{ spawn "true"; }')
    return run_script(SCRIPT.format(runner=HERE / "runners" / "shortcuts.py"), timeout=400, config_kdl=harmless, global_shortcuts=True)


if __name__ == "__main__":
    sys.exit(main())
