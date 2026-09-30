#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import activate, active_title, konveyor_action, managed_titles, qdbus, run_script, wait_for, window_state

GAME = sys.argv[1]
MAKE_FULLSCREEN = "--make-fullscreen" in sys.argv[2:]
ROUNDS = 3


def focused_workspace():
    workspaces = json.loads(qdbus("org.kde.Konveyor", "/Konveyor", "org.kde.Konveyor.Workspaces"))
    return next((workspace["idx"] for workspace in workspaces if workspace["is_focused"]), None)


def switch_desktop(index):
    run_script(f"workspace.currentDesktop = workspace.desktops[{index - 1}];")


def expect(step, index, title, problems):
    wait_for(lambda: focused_workspace() == index and active_title() == title, 60)
    workspace, active = focused_workspace(), active_title()
    print(f"{step}: workspace {workspace}, active {active}")
    if workspace != index or active != title:
        problems.append(f"{step}: expected workspace {index} with {title} active, got workspace {workspace} with {active} active")


def main():
    problems = []
    wait_for(lambda: GAME in managed_titles(), 60)
    activate(GAME)
    if MAKE_FULLSCREEN:
        konveyor_action("fullscreen-window")
    wait_for(lambda: (window_state(GAME) or "").startswith("true|"), 60)
    konveyor_action("move-window-to-workspace", "2")
    expect("moved to workspace 2", 2, GAME, problems)
    state = window_state(GAME)
    print(f"{GAME}: {state}")
    if not state or not state.startswith("true|"):
        problems.append(f"{GAME} is not fullscreen on workspace 2 ({state})")
    for round_ in range(1, ROUNDS + 1):
        konveyor_action("focus-workspace", "1")
        expect(f"round {round_}: focus-workspace 1", 1, "A", problems)
        konveyor_action("focus-workspace", "2")
        expect(f"round {round_}: focus-workspace 2", 2, GAME, problems)
    for round_ in range(1, ROUNDS + 1):
        switch_desktop(1)
        expect(f"round {round_}: KDE switches to desktop 1", 1, "A", problems)
        switch_desktop(2)
        expect(f"round {round_}: KDE switches to desktop 2", 2, GAME, problems)
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
