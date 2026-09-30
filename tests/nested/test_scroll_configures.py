#!/usr/bin/env python3
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

BROWSERS = ("google-chrome-stable", "chromium")

PORTRAIT_WITH_PANEL = """
output "Virtual-0" {
    layout {
        new-window-placement "column"
        default-column-width { proportion 1.0; }
        struts {
            top 42.4
        }
    }
}
"""


def main():
    from nested import run_script

    chrome = next((browser for browser in BROWSERS if shutil.which(browser)), None)
    if chrome is None:
        print(f"SKIP: none of {', '.join(BROWSERS)} is installed")
        return 77
    script = f"""
export QT_QPA_PLATFORM=wayland
export KONVEYOR_CLIENT_LOGS="$(dirname "$KONVEYOR_REPORT")"
kscreen-doctor output.1.scale.1.25 > /dev/null 2>&1
python3 {HERE / "runners" / "scroll_configures.py"} {chrome} > "$KONVEYOR_REPORT" 2>&1
pkill -f "user-data-dir=$KONVEYOR_CLIENT_LOGS/chrome-"
"""
    return run_script(script, timeout=480, extra_config=PORTRAIT_WITH_PANEL, width=1848, height=2960)


if __name__ == "__main__":
    sys.exit(main())
