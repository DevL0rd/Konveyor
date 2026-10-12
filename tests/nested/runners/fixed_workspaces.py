#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, load_config
from kwinsession import for_window, konveyor, konveyor_action, konveyor_windows, open_client, run_script, wait_for


def workspaces():
    return json.loads(konveyor("Workspaces"))


def counts():
    outputs = {workspace["output"] for workspace in workspaces()}
    return {output: sum(workspace["output"] == output for workspace in workspaces()) for output in outputs}


def kde_count():
    return int(run_script('print("MARK|" + workspace.desktops.length);')[0])


def fixed(checks):
    checks.expect(wait_for(lambda: len(counts()) == 2 and set(counts().values()) == {4}), "both monitors start with four slots")
    checks.equal(kde_count(), 4, "KDE has four actual virtual desktops")
    before = {workspace["id"] for workspace in workspaces()}
    checks.equal(konveyor_action("focus-workspace", "4"), "", "focus the last fixed workspace")
    open_client("Fixed A")
    open_client("Fixed B")
    checks.equal(kde_count(), 4, "opening on the last slot adds no KDE desktop")
    checks.equal(konveyor_action("move-window-to-workspace", "99"), "", "out-of-range move clamps to a fixed slot")
    checks.equal(kde_count(), 4, "the numeric move does not grow KDE desktops")
    run_script(for_window("Fixed B", "w.closeWindow();") + for_window("Fixed A", "w.closeWindow();"))
    checks.expect(wait_for(lambda: not konveyor_windows()), "both clients close")
    checks.equal(konveyor_action("focus-workspace", "1"), "", "leave the empty final workspace")
    checks.equal({workspace["id"] for workspace in workspaces()}, before, "closing windows preserves all numbered slot identities")
    checks.equal(kde_count(), 4, "closing windows does not shrink KDE desktops")


def reload(checks):
    checks.expect(load_config('layout { fixed-workspace-count 2; }\n'), "reduce the count live")
    checks.expect(wait_for(lambda: set(counts().values()) == {2}), "both core stacks now have two slots")
    checks.equal(kde_count(), 2, "KDE follows the explicitly reduced count")
    checks.expect(load_config('layout { fixed-workspace-count 0; }\nanimations { off; }\n'), "restore dynamic mode live")
    for output in counts():
        konveyor_action("focus-monitor", output)
        konveyor_action("focus-workspace-down")
    checks.expect(wait_for(lambda: set(counts().values()) == {1}), "dynamic mode prunes empty slots after leaving the retained focused slot")
    checks.equal(kde_count(), 1, "KDE follows the dynamic empty-stack count")
    checks.expect(load_config('layout { fixed-workspace-count 4; }\n'), "restore fixed mode live")
    checks.expect(wait_for(lambda: set(counts().values()) == {4}), "four fixed slots return")
    checks.equal(kde_count(), 4, "KDE has four desktops again")


def main():
    Checks().run(fixed, reload)


if __name__ == "__main__":
    main()
