#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import CLIENTS, for_window, frame, intersects, konveyor, konveyor_action, konveyor_windows, managed_titles, run_script, wait_for

MANAGED_X11 = ["X-Normal", "X-Dialog", "X-Transient", "X-Modal", "X-Sticky"]
UNMANAGED_X11 = ["X-Utility:UTILITY", "X-Splash:SPLASH", "X-Toolbar:TOOLBAR", "X-Menu:MENU", "X-Dropdown:DROPDOWN_MENU:override",
                 "X-PopupMenu:POPUP_MENU:override", "X-Tooltip:TOOLTIP:override", "X-Notification:NOTIFICATION", "X-Dock:DOCK", "X-Desktop:DESKTOP", "X-OSD:ON_SCREEN_DISPLAY",
                 "X-Critical:CRITICAL_NOTIFICATION", "X-AppletPopup:APPLET_POPUP", "X-SkipTaskbar:NORMAL:skip-taskbar"]
X11_SPECS = ["X-Normal:NORMAL", "X-Sticky:NORMAL", "X-Dialog:DIALOG", "X-Transient:NORMAL:transient", "X-Modal:DIALOG:transient:modal", *UNMANAGED_X11]
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


def konveyor_window(title):
    return next((window for window in konveyor_windows() if window["title"] == title), None)


def active_workspace():
    return next(workspace["id"] for workspace in json.loads(konveyor("Workspaces")) if workspace["is_active"])


def on_all_desktops(title):
    return run_script(for_window(title, 'print("MARK|" + w.onAllDesktops);')) == ["true"]


def shows_on_the_active_workspace(title):
    window = konveyor_window(title)
    return window is not None and not window["is_floating"] and window["workspace_id"] == active_workspace() and on_all_desktops(title)


def pinned_to_all_desktops(checks):
    checks.expect(wait_for(lambda: shows_on_the_active_workspace("X-Sticky")), "a window that opened on all desktops is tiled")
    run_script(for_window("X-Normal", "w.onAllDesktops = true;"))
    for action in ("focus-workspace-down", "focus-workspace-down", "focus-workspace-up"):
        konveyor_action(action)
        checks.expect(wait_for(lambda: shows_on_the_active_workspace("X-Normal") and shows_on_the_active_workspace("X-Sticky")),
                      f"windows pinned to all desktops stay tiled on all desktops and follow {action}")
        if action == "focus-workspace-down":
            checks.expect(wait_for(lambda: all(intersects(frame(title), (0, 0, 1920, 1080)) for title in ("X-Normal", "X-Sticky"))),
                          "both pinned windows are on screen on the workspace below")
    run_script(for_window("X-Normal", "w.onAllDesktops = false;"))
    checks.expect(wait_for(lambda: not on_all_desktops("X-Normal")), "unpinning takes the window off the other desktops")
    pinned_workspace = konveyor_window("X-Normal")["workspace_id"]
    konveyor_action("focus-workspace-down")
    checks.expect(wait_for(lambda: active_workspace() != pinned_workspace), "the workspace switch after unpinning happened")
    checks.expect(wait_for(lambda: konveyor_window("X-Normal")["workspace_id"] == pinned_workspace and not on_all_desktops("X-Normal")),
                  "an unpinned window stays on its own workspace")
    checks.expect(shows_on_the_active_workspace("X-Sticky"), "the window still pinned follows the switch")


def main():
    Checks().run(open_everything, managed_types, pinned_to_all_desktops)


if __name__ == "__main__":
    main()
