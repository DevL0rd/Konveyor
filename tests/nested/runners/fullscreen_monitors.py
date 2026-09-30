#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from fakepointer import move
from kwinsession import activate, active_title, for_window, konveyor, konveyor_action, open_client, run_script, wait_for

SECOND = (1920.0, 0.0, 1920.0, 1080.0)


def active_workspaces():
    return {workspace["output"]: workspace["idx"] for workspace in json.loads(konveyor("Workspaces")) if workspace["is_active"]}


def game():
    printed = run_script(for_window("G", 'const g = w.frameGeometry; print("MARK|" + w.fullScreen + "|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height);'))
    fullscreen, *geometry = printed[0].split("|")
    return fullscreen == "true", tuple(float(value) for value in geometry)


def desktop_count():
    return len(run_script('for (const d of workspace.desktops) print("MARK|" + d.id);'))


def expect_state(checks, step, first, second):
    wanted = {"Virtual-0": first, "Virtual-1": second}
    checks.expect(wait_for(lambda: active_workspaces() == wanted), f"{step}: Virtual-0 shows workspace {first}, Virtual-1 workspace {second} ({active_workspaces()})")
    if second == 2:
        checks.expect(wait_for(lambda: game() == (True, SECOND)), f"{step}: G is fullscreen over Virtual-1 ({game()})")
    checks.expect(wait_for(lambda: desktop_count() == max(3, first + 1)), f"{step}: KDE has a desktop per workspace ({desktop_count()})")


def setup(checks):
    move(960, 540, settle=False)
    open_client("A")
    open_client("B")
    move(2880, 540, settle=False)
    open_client("F")
    open_client("G")
    activate("G")
    checks.equal(konveyor_action("fullscreen-window"), "", "make G fullscreen")
    checks.expect(wait_for(lambda: game() == (True, SECOND)), f"G covers Virtual-1 ({game()})")
    checks.equal(konveyor_action("move-window-to-workspace", "2"), "", "move G to workspace 2 of Virtual-1")
    activate("G")
    expect_state(checks, "G on its own workspace", 1, 2)


def switch_rounds(checks):
    for round_ in (1, 2):
        activate("A")
        run_script("workspace.currentDesktop = workspace.desktops[1];")
        expect_state(checks, f"round {round_}: KDE switches Virtual-0 to desktop 2", 2, 2)
        run_script("workspace.currentDesktop = workspace.desktops[0];")
        expect_state(checks, f"round {round_}: KDE switches Virtual-0 back", 1, 2)
        activate("F")
        expect_state(checks, f"round {round_}: activating F shows workspace 1 of Virtual-1", 1, 1)
        checks.equal(konveyor_action("focus-workspace", "2"), "", "focus-workspace 2 on Virtual-1")
        expect_state(checks, f"round {round_}: back to G's workspace", 1, 2)
        checks.expect(wait_for(lambda: active_title() == "G"), f"round {round_}: G is active again ({active_title()})")


def main():
    Checks().run(setup, switch_rounds)


if __name__ == "__main__":
    main()
