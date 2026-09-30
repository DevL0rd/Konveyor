#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import activate, kwin_titles, konveyor_windows, managed_titles, open_client, wait_for

SCREENSAVER = ("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver")


def screensaver(method):
    service, path, interface = SCREENSAVER
    return subprocess.run(["qdbus6", service, path, f"{interface}.{method}"], capture_output=True, text=True).stdout.strip()


def row():
    return {window["title"]: tuple(window["layout"]["pos_in_scrolling_layout"]) for window in konveyor_windows()}


def locked_row(checks):
    activate("B")
    before = row()
    windows_before = set(kwin_titles())
    screensaver("Lock")
    checks.expect(wait_for(lambda: screensaver("GetActive") == "true"), "the screen locks")
    checks.expect(wait_for(lambda: set(kwin_titles()) - windows_before), f"the lock screen window mapped ({kwin_titles()})")
    checks.equal(row(), before, "locking leaves the row as it was and does not tile the lock screen")
    open_client("D")
    checks.equal(sorted(managed_titles()), ["A", "B", "C", "D"], "a window opened while the screen is locked joins the row")
    columns = [position[0] for position in row().values()]
    checks.equal(columns.count(row()["D"][0]), 1, "and gets its own column")


def main():
    Checks().run(locked_row)


if __name__ == "__main__":
    main()
