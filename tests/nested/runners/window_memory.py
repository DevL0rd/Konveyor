#!/usr/bin/env python3
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import konveyor_action, konveyor_windows, open_client, wait_for

MODE = sys.argv[1]
MEMORY = Path(os.environ["XDG_STATE_HOME"]) / "konveyor" / "window-memory.json"
APP = "org.qt-project.qml"


def remembered():
    try:
        return json.loads(MEMORY.read_text()).get(APP, {}).get("column-width")
    except (OSError, ValueError):
        return None


def tile_width(title):
    return next(window["layout"]["tile_size"][0] for window in konveyor_windows() if window["title"] == title)


def log_mentions(text):
    with open(os.environ["KONVEYOR_KWIN_LOG"], errors="replace") as log:
        return any(text in line for line in log)


def opens_at(checks, title, width):
    before = tile_width(title)
    checks.equal(konveyor_action("set-column-width", width), "", f"set-column-width {width} on {title}")
    checks.equal(tile_width(title), before, f"{title} already had width {width}")


def corrupt(checks):
    checks.expect(log_mentions(f'could not read "{MEMORY}"'), "the unreadable window-memory.json is reported in the log")
    open_client("A")
    opens_at(checks, "A", "50%")
    checks.equal(konveyor_action("set-column-width", "75%"), "", "set-column-width 75%")
    checks.expect(wait_for(remembered), "the new width is saved over the unreadable file")
    checks.equal(remembered(), {"proportion": True, "value": 0.75}, "the saved width")
    open_client("B")
    opens_at(checks, "B", "75%")


def read_only(checks):
    MEMORY.parent.chmod(0o555)
    try:
        open_client("A")
        opens_at(checks, "A", "25%")
        checks.equal(konveyor_action("set-column-width", "75%"), "", "set-column-width 75%")
        checks.expect(wait_for(lambda: log_mentions(f'could not write "{MEMORY}"')), "a failed save is reported in the log")
        checks.equal(remembered(), {"proportion": True, "value": 0.25}, "the read-only file is left as it was")
        open_client("B")
        checks.equal(sorted(window["title"] for window in konveyor_windows()), ["A", "B"], "windows keep opening")
    finally:
        MEMORY.parent.chmod(0o755)


def main():
    Checks().run({"corrupt": corrupt, "read-only": read_only}[MODE])


if __name__ == "__main__":
    main()
