#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from kwinsession import (CLIENTS, activate, active_title, for_window, konveyor, konveyor_action, konveyor_windows, managed_now, open_client,
                         qdbus, reload_konveyor, run_script, wait_for, window_state)

def reload_effect(checks, titles):
    checks.expect(reload_konveyor(titles), f"the reloaded effect manages the open windows ({sorted(managed_now())})")


def layout():
    indexes = {workspace["id"]: workspace["idx"] for workspace in json.loads(konveyor("Workspaces"))}
    return {window["title"]: (indexes[window["workspace_id"]], window["is_floating"], window["layout"]["pos_in_scrolling_layout"],
                              window["layout"]["tile_size"][0]) for window in konveyor_windows()}


def desktop_of(title):
    return run_script(for_window(title, 'print("MARK|" + workspace.desktops.indexOf(w.desktops[0]));'))[0]


def arrange(checks):
    for title in ("D", "E", "F"):
        open_client(title)
    steps = [("C", "move-column-left"), ("C", "move-column-left"), ("D", "consume-or-expel-window-left"), ("A", "set-column-width", "30%"),
             ("F", "toggle-window-floating"), ("E", "move-window-to-workspace", "2"), ("E", "focus-workspace", "1"), ("B", None)]
    for title, *action in steps:
        activate(title)
        if action[0]:
            checks.equal(konveyor_action(*action), "", f"{' '.join(action)} on {title}")
    checks.expect(wait_for(lambda: layout()["E"][0] == 2 and layout()["D"][2][0] == layout()["B"][2][0] and layout()["F"][1]),
                  f"the windows are arranged ({layout()})")


def layout_survives(checks):
    arrange(checks)
    before = layout()
    reload_effect(checks, before)
    checks.expect(wait_for(lambda: layout() == before, 10), f"columns, stacks, widths, floating and workspaces survive the reload ({before} -> {layout()})")
    checks.equal(active_title(), "B", "the focused window keeps focus")
    checks.equal(desktop_of("E"), "1", "E stays on the second KDE desktop")
    run_script(for_window("C", "w.desktops = [workspace.desktops[1]];"))
    checks.expect(wait_for(lambda: layout()["C"][0] == 2), "moving a window that was open before the reload to another KDE desktop moves it to that workspace")


def minimized(title, value):
    run_script(for_window(title, f"w.minimized = {'true' if value else 'false'};"))


def minimized_window_survives(checks):
    before = layout()
    minimized("A", True)
    checks.expect(wait_for(lambda: "A" not in layout()), "minimizing A takes it out of the layout")
    reload_effect(checks, [title for title in before if title != "A"])
    minimized("A", False)
    checks.expect(wait_for(lambda: layout().get("A") == before["A"], 10),
                  f"A comes back where it was after a reload while minimized ({before['A']} -> {layout().get('A')})")


def rules_apply_on_load(checks):
    rule = '\nwindow-rule {\n    match title="^Ruled$"\n    open-floating true\n}\n'
    checks.expect(load_config(default_config() + rule), "a config with an open-floating rule loads")
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.unloadEffect", "konveyor_effect")
    open_client("Ruled", managed=False)
    checks.expect(reload_konveyor(["Ruled"]), "the effect loads with the window already open")
    ruled = next(window for window in konveyor_windows() if window["title"] == "Ruled")
    checks.expect(ruled["is_floating"], "a window open before the effect loaded follows the config's open-floating rule")
    load_config(default_config())


def fullscreen(title):
    return (window_state(title) or "").startswith("true|")


def fullscreen_survives(checks):
    subprocess.Popen(["qml6", str(CLIENTS / "fullscreen-client.qml"), "--", "/nonexistent"])
    subprocess.Popen(["python3", str(CLIENTS / "x11-types.py"), "X11Full:NORMAL"])
    checks.expect(wait_for(lambda: "X11Full" in managed_now(), 60), "the X11 window opened")
    run_script(for_window("X11Full", "w.fullScreen = true;"))
    titles = ("SelfFullscreen", "X11Full")
    checks.expect(wait_for(lambda: all(fullscreen(title) for title in titles), 60), "the Wayland and X11 windows made themselves fullscreen")
    reload_effect(checks, titles)
    for title in titles:
        checks.expect(wait_for(lambda: fullscreen(title), 10) and not wait_for(lambda: not fullscreen(title), 3),
                      f"{title} is still fullscreen after the effect reloaded ({window_state(title)})")


def desktop_names():
    return run_script('for (const d of workspace.desktops) print("MARK|" + d.name);')


def unload_gives_back_desktops(checks):
    print("desktops while Konveyor runs:", desktop_names())
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.unloadEffect", "konveyor_effect")
    checks.equal(desktop_names(), ["Mail", "Web", "Chat", "Music"], "unloading gives back KDE's four desktops with their names")


def main():
    Checks().run(layout_survives, minimized_window_survives, rules_apply_on_load, fullscreen_survives, unload_gives_back_desktops)


if __name__ == "__main__":
    main()
