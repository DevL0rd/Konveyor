#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, load_config
from kwinsession import konveyor_action, open_client, qdbus, run_script, wait_for


def rows():
    return int(qdbus("org.kde.KWin", "/VirtualDesktopManager", "org.freedesktop.DBus.Properties.Get", "org.kde.KWin.VirtualDesktopManager", "rows"))


def desktop_count():
    return int(run_script('print("MARK|" + workspace.desktops.length);')[0])


def horizontal(checks):
    checks.equal(konveyor_action("focus-workspace", "1"), "", "start on the first workspace")
    open_client("Horizontal A")
    checks.expect(wait_for(lambda: desktop_count() == 2), "an occupied workspace and the empty one give two desktops")
    checks.equal(rows(), 1, "the desktops sit in one row")
    konveyor_action("focus-workspace", "2")
    open_client("Horizontal B")
    checks.expect(wait_for(lambda: desktop_count() == 3), "a third desktop appears")
    checks.equal(rows(), 1, "adding a desktop keeps the one row")


def toggle(checks):
    checks.expect(load_config("layout { }\n"), "turn horizontal desktops off live")
    checks.expect(wait_for(lambda: rows() == desktop_count()), "the desktops go back to one column")
    checks.expect(load_config("layout { horizontal-desktops; }\n"), "turn them on again live")
    checks.expect(wait_for(lambda: rows() == 1), "the desktops return to one row")


def main():
    Checks().run(horizontal, toggle)


if __name__ == "__main__":
    main()
