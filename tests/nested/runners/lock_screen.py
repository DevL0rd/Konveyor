#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import activate, konveyor_windows, managed_titles, open_client, run_script, wait_for

SCREENSAVER = ("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver")


def screensaver(method):
    service, path, interface = SCREENSAVER
    return subprocess.run(["qdbus6", service, path, f"{interface}.{method}"], capture_output=True, text=True).stdout.strip()


def row():
    return {window["title"]: tuple(window["layout"]["pos_in_scrolling_layout"]) for window in konveyor_windows()}


def mapped_lock_window_ids():
    return set(run_script('for (const w of workspace.windowList()) { const g = w.frameGeometry; '
                          'const output = workspace.clientArea(KWin.FullScreenArea, w); '
                          'const background = w.resourceClass == "kwin_wayland" && w.skipTaskbar && !w.outline '
                          '&& g.x == output.x && g.y == output.y && g.width == output.width && g.height == output.height; '
                          'const greeter = w.resourceClass == "kscreenlocker_greet"; '
                          'if (!w.deleted && !w.hidden && g.width > 0 && g.height > 0 && (background || greeter)) '
                          'print("MARK|" + w.internalId); }'))


def locked_row(checks):
    activate("B")
    checks.equal(screensaver("GetActive"), "false", "the nested screen starts unlocked")
    before = row()
    windows_before = mapped_lock_window_ids()
    screensaver("Lock")
    checks.expect(wait_for(lambda: screensaver("GetActive") == "true"), "the screen locks")
    checks.expect(wait_for(lambda: mapped_lock_window_ids() - windows_before), "the lock screen window mapped with a new identity and positive geometry")
    checks.equal(row(), before, "locking leaves the row as it was and does not tile the lock screen")
    open_client("D")
    checks.equal(sorted(managed_titles()), ["A", "B", "C", "D"], "a window opened while the screen is locked joins the row")
    columns = [position[0] for position in row().values()]
    checks.equal(columns.count(row()["D"][0]), 1, "and gets its own column")


def main():
    Checks().run(locked_row)


if __name__ == "__main__":
    main()
