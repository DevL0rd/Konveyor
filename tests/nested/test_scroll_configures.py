#!/usr/bin/env python3
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

CHROME = "google-chrome-stable"

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

    if shutil.which(CHROME) is None:
        print(f"SKIP: {CHROME} is not installed")
        return 77
    launches = "\n".join(
        f'mkdir -p "$KONVEYOR_CLIENT_LOGS/chrome-{label}"\n'
        f'WAYLAND_DEBUG=client {CHROME} --ozone-platform=wayland --no-first-run --no-default-browser-check --disable-sync --password-store=basic '
        f'--user-data-dir="$KONVEYOR_CLIENT_LOGS/chrome-{label}" "data:text/html,<title>{label}</title>{label}" > /dev/null 2> "$KONVEYOR_CLIENT_LOGS/{label}.log" &\n'
        f"sleep 6"
        for label in "abcdef"
    )
    script = f"""
export QT_QPA_PLATFORM=wayland
export KONVEYOR_CLIENT_LOGS="$(dirname "$KONVEYOR_REPORT")"
kscreen-doctor output.1.scale.1.25 > /dev/null 2>&1
sleep 2
{launches}
sleep 4
python3 {HERE / "runners" / "scroll_configures.py"} > "$KONVEYOR_REPORT" 2>&1
pkill -f "user-data-dir=$KONVEYOR_CLIENT_LOGS/chrome-"
"""
    return run_script(script, timeout=480, extra_config=PORTRAIT_WITH_PANEL, width=1848, height=2960)


if __name__ == "__main__":
    sys.exit(main())
