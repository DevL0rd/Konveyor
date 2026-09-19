#!/usr/bin/env python3
import json
import math
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import activate, konveyor_action, konveyor_windows, run_script


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


def wait_for_windows():
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        state = snapshot()
        if all(name in state.get("windows", {}) for name in WANTED):
            return state
        time.sleep(0.1)
    return snapshot()


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


def main():
    problems = []
    state = wait_for_windows()
    if not all(name in state.get("windows", {}) for name in WANTED):
        missing = [name for name in WANTED if name not in state.get("windows", {})]
        problems.append(f"windows did not appear: {missing}")
    else:
        managed = {window["title"] for window in konveyor_windows()}
        if managed != {TARGET, COVER}:
            problems.append(f"overlay windows entered the layout: {sorted(managed)}")
        check_geometry(state, problems, "initial")
        check_ownership(state, problems, "initial")

        activate(COVER)
        time.sleep(0.5)
        state = snapshot()
        check_owner_stack(state, problems, "cover focused")
        if any(state["windows"][name]["index"] >= state["windows"][COVER]["index"] for name in [TARGET, *CARDS, PANEL]):
            problems.append("cover focused: owner overlay group remained above the focused cover")

        activate(TARGET)
        time.sleep(0.5)
        state = snapshot()
        check_owner_stack(state, problems, "owner refocused")

        konveyor_action("set-column-width", "75%")
        time.sleep(1.0)
        state = snapshot()
        check_geometry(state, problems, "resized")

        konveyor_action("fullscreen-window")
        time.sleep(1.0)
        state = snapshot()
        if not state["windows"][TARGET]["fullscreen"]:
            problems.append("fullscreen: target did not enter fullscreen")
        check_geometry(state, problems, "fullscreen")
        check_owner_stack(state, problems, "fullscreen")

        activate(COVER)
        time.sleep(0.5)
        activate(TARGET)
        time.sleep(0.5)
        state = snapshot()
        check_geometry(state, problems, "fullscreen focus round trip")
        check_owner_stack(state, problems, "fullscreen focus round trip")

    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
