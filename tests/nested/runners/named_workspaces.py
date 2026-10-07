#!/usr/bin/env python3
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import activate, konveyor, konveyor_action, konveyor_windows, open_client, run_script, wait_for, watch_signals

TITLES = ("A", "B", "C")
RENAMES = "type='signal',interface='org.kde.KWin.VirtualDesktopManager',member='desktopDataChanged'"
SCROLLS = ("focus-column-left", "focus-column-left", "focus-column-right", "focus-column-right") * 2


def workspaces():
    return json.loads(konveyor("Workspaces"))


def active_workspaces():
    return {workspace["output"]: (workspace["idx"], workspace["name"] or "") for workspace in workspaces() if workspace["is_active"]}


def output_of(title):
    outputs = {workspace["id"]: workspace["output"] for workspace in workspaces()}
    return next(outputs[window["workspace_id"]] for window in konveyor_windows() if window["title"] == title)


def desktop_names():
    return run_script('for (const d of workspace.desktops) print("MARK|" + d.name);')


def scroll():
    for action in SCROLLS:
        konveyor_action(action)
        time.sleep(0.25)


def renames_while_scrolling():
    time.sleep(1)
    return [line for line in watch_signals(RENAMES, scroll, lambda _: False, timeout=len(SCROLLS) * 0.25 + 2) if "desktopDataChanged" in line]


def any_monitor(checks):
    checks.equal(konveyor_action("focus-workspace", "web"), "", "focus the named workspace that opens on any monitor")
    other = next(output for output, (_, name) in active_workspaces().items() if name != "web")
    for title in TITLES:
        open_client(title)
    for title in TITLES:
        activate(title)
        konveyor_action("move-column-to-monitor", other)
    wait_for(lambda: {output_of(title) for title in TITLES} == {other}, 30)
    activate("B")
    active = active_workspaces()
    print(f"active workspaces: {active}")
    checks.equal(sorted(active.values()), [(1, ""), (1, "web")], "one output shows the named workspace, the other an unnamed one")
    checks.equal(len(renames_while_scrolling()), 0, "scrolling beside the named workspace does not rename virtual desktops")
    checks.equal(desktop_names()[0], "web", "the shared virtual desktop carries the name of the named workspace")


def both_monitors(checks):
    checks.equal(konveyor_action("set-workspace-name", "chat"), "", "name the workspace that is being scrolled")
    active = active_workspaces()
    print(f"active workspaces: {active}")
    checks.equal(sorted(active.values()), [(1, "chat"), (1, "web")], "both outputs show a named first workspace")
    checks.equal(len(renames_while_scrolling()), 0, "scrolling beside another named workspace does not rename virtual desktops")
    checks.equal(desktop_names()[0], "chat", "the focused output names the shared virtual desktop")


def main():
    wait_for(lambda: len({workspace["output"] for workspace in workspaces()}) == 2, 60)
    Checks().run(any_monitor, both_monitors)


if __name__ == "__main__":
    main()
