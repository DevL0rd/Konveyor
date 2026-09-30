#!/usr/bin/env python3
import configparser
import json
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, config_path, default_config
from kwinsession import activate, for_window, konveyor, konveyor_action, konveyor_windows, qdbus, run_script, wait_for

RULES = Path(os.environ["XDG_CONFIG_HOME"]) / "kwinrulesrc"
RULE = "konveyor-no-minimize"
USER_RULE = "user-rule"


def desktops():
    return [line.split("|", 1) for line in run_script('for (const d of workspace.desktops) print("MARK|" + d.id + "|" + d.name);')]


def desktop_names():
    return [name for _, name in desktops()]


def rows():
    return int(qdbus("org.kde.KWin", "/VirtualDesktopManager", "org.freedesktop.DBus.Properties.Get", "org.kde.KWin.VirtualDesktopManager", "rows"))


def window_desktops(title):
    return run_script(for_window(title, 'print("MARK|" + w.desktops.map(d => d.id).join(","));'))[0].split(",")


def workspace_of(title):
    indexes = {workspace["id"]: workspace["idx"] for workspace in json.loads(konveyor("Workspaces"))}
    return next(indexes[window["workspace_id"]] for window in konveyor_windows() if window["title"] == title)


def focused_workspace():
    return next(workspace["idx"] for workspace in json.loads(konveyor("Workspaces")) if workspace["is_focused"])


def rules():
    parser = configparser.ConfigParser(interpolation=None, strict=False)
    parser.optionxform = str
    parser.read(RULES)
    return parser


def listed_rules():
    parser = rules()
    return parser.get("General", "rules", fallback="").split(",") if parser.has_section("General") else []


def minimizable(title):
    return run_script(for_window(title, 'print("MARK|" + w.minimizable);')) == ["true"]


def desktop_count(checks):
    checks.expect(wait_for(lambda: len(desktops()) == 2), f"KDE has one desktop per workspace, plus the empty one ({desktop_names()})")
    checks.equal(rows(), 2, "the desktops sit in one column")
    checks.equal(konveyor_action("move-window-to-workspace", "2"), "", "move C to workspace 2")
    checks.expect(wait_for(lambda: len(desktops()) == 3), f"a new workspace adds a KDE desktop ({desktop_names()})")
    checks.expect(wait_for(lambda: window_desktops("C") == [desktops()[1][0]]), "C is on KDE desktop 2")
    checks.equal(rows(), 3, "rows follow the desktop count")
    activate("C")
    checks.equal(konveyor_action("move-window-to-workspace", "1"), "", "move C back")
    checks.expect(wait_for(lambda: len(desktops()) == 2), f"the emptied workspace removes its KDE desktop ({desktop_names()})")


def user_moves(checks):
    run_script(for_window("B", "w.desktops = [workspace.desktops[1]];"))
    checks.expect(wait_for(lambda: workspace_of("B") == 2), "moving B to KDE desktop 2 moves it to workspace 2")
    checks.expect(wait_for(lambda: len(desktops()) == 3), "and adds a desktop for the new empty workspace")
    run_script("workspace.currentDesktop = workspace.desktops[1];")
    checks.expect(wait_for(lambda: focused_workspace() == 2), "switching KDE to desktop 2 focuses workspace 2")
    run_script("workspace.currentDesktop = workspace.desktops[0];")
    checks.expect(wait_for(lambda: focused_workspace() == 1), "switching KDE back focuses workspace 1")


def named_indexes():
    return {workspace["name"]: workspace["idx"] for workspace in json.loads(konveyor("Workspaces")) if workspace["name"]}


def names(checks):
    home = workspace_of("A")
    config_path().write_text(default_config() + '\nworkspace "web"\nworkspace "chat"\n')
    checks.expect(wait_for(lambda: set(named_indexes()) == {"web", "chat"}), "the named workspaces exist")
    indexes = named_indexes()
    for name in ("web", "chat"):
        checks.equal(konveyor_action("focus-workspace", name), "", f"focus-workspace {name}")
        checks.expect(wait_for(lambda: desktop_names()[indexes[name] - 1] == name), f"{name} names its KDE desktop ({desktop_names()})")
    checks.equal(konveyor_action("focus-workspace", str(workspace_of("A"))), "", "back to the windows")
    config_path().write_text(default_config())
    checks.expect(wait_for(lambda: not named_indexes()), "the names are gone from the config")
    for name in ("web", "chat"):
        checks.equal(konveyor_action("focus-workspace", str(indexes[name])), "", f"focus the workspace that was {name}")
        checks.expect(wait_for(lambda: desktop_names()[indexes[name] - 1] == f"Desktop {indexes[name]}"),
                      f"the desktop that was {name} gets its KDE name back ({desktop_names()})")
    checks.equal(konveyor_action("focus-workspace", str(workspace_of("A"))), "", "back to the windows")
    checks.expect(wait_for(lambda: len(desktops()) == max(workspace["idx"] for workspace in json.loads(konveyor("Workspaces")))),
                  f"one KDE desktop per workspace ({desktop_names()})")
    print("windows started on workspace", home)


def minimize_rule(checks):
    checks.equal(listed_rules(), [USER_RULE], "without disable-minimize only the user's rule is listed")
    config_path().write_text(default_config() + "\ndisable-minimize\n")
    checks.expect(wait_for(lambda: listed_rules() == [USER_RULE, RULE]), f"disable-minimize adds the rule ({listed_rules()})")
    checks.equal(dict(rules()[RULE]) if rules().has_section(RULE) else {}, {"Description": "Konveyor: windows cannot be minimized", "minimize": "false",
                 "minimizerule": "2"}, "the rule forces minimize off")
    checks.expect(wait_for(lambda: not minimizable("A")), "windows can no longer be minimized")
    config_path().write_text(default_config())
    checks.expect(wait_for(lambda: listed_rules() == [USER_RULE] and not rules().has_section(RULE)), "removing disable-minimize removes the rule")
    checks.expect(wait_for(lambda: minimizable("A")), "windows can be minimized again")
    checks.equal(rules().get(USER_RULE, "title", fallback=None), "keep me", "the user's rule is untouched")
    parser = rules()
    parser[RULE] = {"Description": "stale", "minimize": "false", "minimizerule": "2"}
    with RULES.open("w") as file:
        parser.write(file, space_around_delimiters=False)
    config_path().write_text(default_config() + "\n")
    checks.expect(wait_for(lambda: not rules().has_section(RULE)), "a stale unlisted rule group is cleaned up on reload")
    parser = rules()
    parser["General"]["rules"] = f"{USER_RULE},{RULE}"
    with RULES.open("w") as file:
        parser.write(file, space_around_delimiters=False)
    config_path().write_text(default_config() + "\ndisable-minimize\n")
    checks.expect(wait_for(lambda: rules().has_section(RULE) and listed_rules() == [USER_RULE, RULE]),
                  f"a listed rule without its group is written out again ({listed_rules()})")
    checks.expect(wait_for(lambda: not minimizable("A")), "and blocks minimizing")


def unload(checks):
    before = subprocess.run(["sha256sum", str(config_path())], capture_output=True, text=True).stdout.split()[0]
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.unloadEffect", "konveyor_effect")
    checks.expect(wait_for(lambda: "org.kde.Konveyor" not in qdbus("org.freedesktop.DBus", "/", "org.freedesktop.DBus.ListNames")),
                  "unloading releases org.kde.Konveyor")
    checks.equal(rows(), 1, "unloading restores the desktop rows")
    checks.expect(not rules().has_section(RULE) and listed_rules() == [USER_RULE], "unloading removes the minimize rule")
    checks.expect(wait_for(lambda: minimizable("A")), "windows can be minimized after unloading")
    checks.equal(sorted(title for title in run_script('for (const w of workspace.windowList()) { if (w.normalWindow) print("MARK|" + w.caption); }')),
                 ["A", "B", "C"], "the windows are still open")
    after = subprocess.run(["sha256sum", str(config_path())], capture_output=True, text=True).stdout.split()[0]
    checks.equal(after, before, "unloading leaves config.kdl alone")


def main():
    Checks().run(desktop_count, user_moves, names, minimize_rule, unload)


if __name__ == "__main__":
    main()
