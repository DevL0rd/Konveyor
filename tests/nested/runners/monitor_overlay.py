#!/usr/bin/env python3
import json
import math
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import CLIENTS, activate, konveyor_action, konveyor_windows, open_client, run_script, wait_for


TARGET = "Target"
COVER = "Cover"
CARDS = [f"Konveyor Monitor Overlay 1 {slot}" for slot in range(3)]
PANEL = "Konveyor Monitor Panel 1 1"
WANTED = [TARGET, COVER, *CARDS, PANEL]


def snapshot():
    names = json.dumps(WANTED)
    body = f"""
const names = {names};
const order = workspace.stackingOrder;
const out = {{active: workspace.activeWindow ? workspace.activeWindow.caption : null, order: order.map(w => w.caption), windows: {{}}}};
for (const w of workspace.windowList()) {{
    if (!names.includes(w.caption)) continue;
    const g = w.frameGeometry;
    out.windows[w.caption] = {{x:g.x, y:g.y, width:g.width, height:g.height, fullscreen:w.fullScreen,
        parent:w.transientFor ? w.transientFor.caption : null, skipTaskbar:w.skipTaskbar, index:order.indexOf(w)}};
}}
print("MARK|" + JSON.stringify(out));
"""
    printed = run_script(body)
    return json.loads(printed[0]) if printed else {}


def open_windows():
    open_client(TARGET, 1000, 700)
    open_client(COVER, 700, 500)
    with open(os.environ["KONVEYOR_KWIN_LOG"], "a") as log:
        for name, width, height in ((CARDS[0], 180, 32), (CARDS[1], 260, 32), (CARDS[2], 220, 32), (PANEL, 500, 360)):
            subprocess.Popen(["qml6", str(CLIENTS / "overlay-client.qml"), "--", name, str(width), str(height)], stdout=log, stderr=subprocess.STDOUT)
    wait_for(lambda: all(name in snapshot().get("windows", {}) for name in WANTED), 60)
    return snapshot()


def settled(label, *checks):
    def found():
        state = snapshot()
        problems = []
        for check in checks:
            check(state, problems, label)
        return problems

    wait_for(lambda: not found(), 60)
    return found()


def rounded(value):
    return math.floor(value + 0.5)


def check_geometry(state, problems, label):
    windows = state["windows"]
    target = windows[TARGET]
    cards = [windows[name] for name in CARDS]
    total = sum(card["width"] for card in cards)
    expected_x = rounded(target["x"] + (target["width"] - total) / 2)
    for name, card in zip(CARDS, cards):
        if (card["x"], card["y"]) != (expected_x, target["y"]):
            problems.append(f"{label}: {name} at {card['x']},{card['y']} expected {expected_x},{target['y']}")
        expected_x += card["width"]
    panel = windows[PANEL]
    panel_x = rounded(target["x"] + (target["width"] - panel["width"]) / 2)
    panel_y = rounded(target["y"] + (target["height"] - panel["height"]) / 2)
    if (panel["x"], panel["y"]) != (panel_x, panel_y):
        problems.append(f"{label}: panel at {panel['x']},{panel['y']} expected {panel_x},{panel_y}")


def check_ownership(state, problems, label):
    for name in [*CARDS, PANEL]:
        window = state["windows"][name]
        if window["parent"] != TARGET:
            problems.append(f"{label}: {name} parent is {window['parent']}")
        if not window["skipTaskbar"]:
            problems.append(f"{label}: {name} is visible in the taskbar")


def check_owner_stack(state, problems, label):
    windows = state["windows"]
    for name in [*CARDS, PANEL]:
        if windows[name]["index"] <= windows[TARGET]["index"]:
            problems.append(f"{label}: {name} is not above its owner")


def check_cover_on_top(state, problems, label):
    if any(state["windows"][name]["index"] >= state["windows"][COVER]["index"] for name in [TARGET, *CARDS, PANEL]):
        problems.append(f"{label}: owner overlay group remained above the focused cover")


def check_fullscreen(state, problems, label):
    if not state["windows"][TARGET]["fullscreen"]:
        problems.append(f"{label}: target did not enter fullscreen")


def main():
    problems = []
    state = open_windows()
    if not all(name in state.get("windows", {}) for name in WANTED):
        missing = [name for name in WANTED if name not in state.get("windows", {})]
        problems.append(f"windows did not appear: {missing}")
    else:
        managed = {window["title"] for window in konveyor_windows()}
        if managed != {TARGET, COVER}:
            problems.append(f"overlay windows entered the layout: {sorted(managed)}")
        problems += settled("initial", check_geometry, check_ownership)

        activate(COVER)
        problems += settled("cover focused", check_owner_stack, check_cover_on_top)

        activate(TARGET)
        problems += settled("owner refocused", check_owner_stack)

        konveyor_action("set-column-width", "75%")
        problems += settled("resized", check_geometry)

        konveyor_action("fullscreen-window")
        problems += settled("fullscreen", check_fullscreen, check_geometry, check_owner_stack)

        activate(COVER)
        activate(TARGET)
        problems += settled("fullscreen focus round trip", check_geometry, check_owner_stack)

    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
