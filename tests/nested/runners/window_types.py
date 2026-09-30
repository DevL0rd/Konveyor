#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import CLIENTS, for_window, konveyor_windows, managed_titles, run_script, wait_for

MANAGED_X11 = ["X-Normal", "X-Dialog", "X-Transient", "X-Modal"]
UNMANAGED_X11 = ["X-Utility:UTILITY", "X-Splash:SPLASH", "X-Toolbar:TOOLBAR", "X-Menu:MENU", "X-Dropdown:DROPDOWN_MENU:override",
                 "X-PopupMenu:POPUP_MENU:override", "X-Tooltip:TOOLTIP:override", "X-Notification:NOTIFICATION", "X-Dock:DOCK", "X-Desktop:DESKTOP", "X-OSD:ON_SCREEN_DISPLAY",
                 "X-Critical:CRITICAL_NOTIFICATION", "X-AppletPopup:APPLET_POPUP", "X-SkipTaskbar:NORMAL:skip-taskbar", "X-Sticky:NORMAL"]
X11_SPECS = ["X-Normal:NORMAL", "X-Dialog:DIALOG", "X-Transient:NORMAL:transient", "X-Modal:DIALOG:transient:modal", *UNMANAGED_X11]
DESCRIBE = ('print("MARK|" + JSON.stringify({title: w.caption, popup: w.popupWindow, dialog: w.dialog, normal: w.normalWindow, '
            'skip: w.skipTaskbar, sticky: w.onAllDesktops, transient: w.transient, app: w.resourceClass}));')


def kwin_windows():
    return [json.loads(line) for line in run_script(f"for (const w of workspace.windowList()) {{ if (!w.deleted) {{ {DESCRIBE} }} }}")]


def titles_in_kwin():
    return {window["title"] for window in kwin_windows()}


def open_everything(checks):
    subprocess.Popen(["python3", str(CLIENTS / "x11-types.py"), *X11_SPECS])
    subprocess.Popen(["qml6", str(CLIENTS / "types-client.qml")])
    expected = {spec.split(":")[0] for spec in X11_SPECS if "override" not in spec} | {"W-Main", "W-Transient", "W-Modal"}
    checks.expect(wait_for(lambda: expected <= titles_in_kwin(), 60), f"every test window mapped ({sorted(expected - titles_in_kwin())} missing)")
    checks.expect(wait_for(lambda: {"X-Normal", "W-Main"} <= set(managed_titles()), 30), "the normal windows joined the layout")
    checks.expect(wait_for(lambda: any(window["popup"] for window in kwin_windows())), "the Wayland popup mapped")
    checks.expect(wait_for(lambda: any(window["app"] == "qml" and window["title"] == "" for window in kwin_windows())), "the layer-shell surface mapped")
    for window in kwin_windows():
        print("   ", window)


def managed_types(checks):
    checks.equal(sorted(managed_titles()), sorted(MANAGED_X11 + ["W-Main", "W-Transient", "W-Modal"]),
                 "only normal windows, dialogs and transients are managed, not popups, layer-shell or special types")
    floating = {window["title"]: window["is_floating"] for window in konveyor_windows()}
    checks.equal(sorted(title for title, value in floating.items() if value), ["W-Modal", "W-Transient", "X-Dialog", "X-Modal", "X-Transient"],
                 "dialogs and transient windows float")


def pinned_to_all_desktops(checks):
    run_script(for_window("X-Normal", "w.onAllDesktops = true;"))
    checks.expect(wait_for(lambda: "X-Normal" not in managed_titles()), "a window pinned to all desktops leaves the layout")
    checks.expect(run_script(for_window("X-Normal", 'print("MARK|" + w.onAllDesktops);')) == ["true"], "it stays on all desktops")
    checks.expect(run_script(for_window("X-Sticky", "w.onAllDesktops = false;")) == [], "unpinning a window that started pinned")
    checks.expect(wait_for(lambda: "X-Sticky" in managed_titles()), "a window unpinned from all desktops joins the layout")
    run_script(for_window("X-Normal", "w.onAllDesktops = false;"))
    checks.expect(wait_for(lambda: "X-Normal" in managed_titles()), "unpinning brings the window back")


def main():
    Checks().run(open_everything, managed_types, pinned_to_all_desktops)


if __name__ == "__main__":
    main()
