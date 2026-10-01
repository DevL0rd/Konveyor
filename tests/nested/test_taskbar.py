#!/usr/bin/env python3
import os
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

CONFIG = '''animations { off; }
layout { default-column-width { proportion 0.3; }; new-column-position "right"; group-app-windows "off"; }
window-rule { match app-id="^org.kde.plasmawindowed$"; open-floating true; }
'''


def main(xwayland=False):
    from nested import NestedSession

    rules = """[General]
count=1
rules=1
[1]
Description=Isolated taskbar host
wmclass=org.kde.plasmawindowed
wmclassmatch=1
acceptfocus=false
acceptfocusrule=2
skiptaskbar=true
skiptaskbarrule=2
above=true
aboverule=2
position=24,16
positionrule=2
size=480,64
sizerule=2
"""
    session = NestedSession(config_kdl=CONFIG, xwayland=xwayland, files={"config/kwinrulesrc": rules})
    report = session.root / "report.txt"
    runner = HERE / "runners" / "taskbar.py"
    script = f'export KONVEYOR_TASKBAR_CLIENT_PLATFORM={"xcb" if xwayland else "wayland"}\nexport QT_QPA_PLATFORM=wayland\nexport KONVEYOR_KWIN_LOG="{session.log_path}"\npython3 -u "{runner}" > "{report}" 2>&1\n'
    try:
        print(f"Isolated taskbar session: {session.root}", flush=True)
        session.start(script)
        try:
            session.wait(timeout=240)
        except Exception as error:
            print(f"Taskbar session did not finish: {error}")
        text = report.read_text() if report.exists() else "no taskbar report"
        print(text.strip())
        passed = "RESULT: PASS" in text
        if not passed:
            print("\n".join(session.log().splitlines()[-45:]))
        destination = os.environ.get("KONVEYOR_TASKBAR_ARTIFACTS")
        if destination:
            target = Path(destination)
            target.mkdir(parents=True, exist_ok=True)
            for source in [report, session.log_path, session.root / "widget.log", *session.root.glob("taskbar-*.png")]:
                if source.exists():
                    shutil.copy(source, target / source.name)
        return 0 if passed else 1
    finally:
        session.cleanup()


if __name__ == "__main__":
    sys.exit(main(xwayland=os.environ.get("KONVEYOR_TASKBAR_X11") == "1"))
