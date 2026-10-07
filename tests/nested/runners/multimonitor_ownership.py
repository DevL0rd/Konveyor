#!/usr/bin/env python3
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from fakepointer import move
from kwinsession import active_title, for_window, frames, konveyor, konveyor_action, konveyor_windows, open_client, run_script, wait_for
from screenshot import capture_workspace, close

GREEN = (0x00, 0xC8, 0x00)
ROOT = Path(os.environ["KONVEYOR_TEST_ROOT"])


def outputs():
    printed = run_script('for (const o of workspace.screens) { const g = o.geometry; print("MARK|" + o.name + "|" + g.x + "|" + g.width); }')
    result = {}
    for line in printed:
        name, x, width = line.split("|")
        result[name] = (float(x), float(width))
    return result


def kwin_output(title):
    printed = run_script(for_window(title, 'print("MARK|" + (w.output ? w.output.name : "none"));'))
    return printed[0] if printed else None


def active_output():
    printed = run_script('print("MARK|" + workspace.activeScreen.name);')
    return printed[0] if printed else None


def workspaces():
    return json.loads(konveyor("Workspaces"))


def active_index(output):
    return next((workspace["idx"] for workspace in workspaces() if workspace["output"] == output and workspace["is_active"]), None)


def engine_output(title):
    window = next((window for window in konveyor_windows() if window["title"] == title), None)
    return next((workspace["output"] for workspace in workspaces() if window and workspace["id"] == window["workspace_id"]), None)


def engine_focus():
    return next((window.get("title") for window in konveyor_windows() if window.get("is_focused")), None)


def settled(title):
    window = next((window for window in konveyor_windows() if window["title"] == title), None)
    frame = frames().get(title)
    return bool(window and frame) and abs(frame[0] - window["layout"]["tile_pos_in_workspace_view"][0]) < 1 and abs(
        frame[2] - window["layout"]["tile_size"][0]) < 1


def all_settled():
    return all(settled(title) for title in ("A", "B"))


def green_columns_on(left, width):
    image = capture_workspace(str(ROOT / "ownership.png"))
    scale = image.width / max(x + w for x, w in outputs().values())
    y = int(image.height / 2)
    return sum(1 for x in range(int(left * scale), int((left + width) * scale), 4) if close(image.getpixel((x, y)), GREEN))


def check_owned(step, left, problems):
    home = outputs()[left]
    shown = wait_for(lambda: green_columns_on(*home), 10)
    print(f"{step}: B frame={frames().get('B')} engine output={engine_output('B')} KWin output={kwin_output('B')} "
          f"sampled B pixels on {left}={shown}")
    if engine_output("B") != left:
        problems.append(f"{step}: the layout moved B to {engine_output('B')}")
    if kwin_output("B") != left:
        problems.append(f"{step}: KWin gives B to {kwin_output('B')} although it lives in {left}'s row")
    if not shown:
        problems.append(f"{step}: the part of B on {left} is not drawn")


def check_right_untouched(step, left, right, problems):
    wait_for(all_settled, 60)
    index, screen, focused = active_index(right), active_output(), engine_focus()
    print(f"{step}: {right} workspace={index} active output={screen} engine focus={focused} KWin focus={active_title()}")
    if index != 2:
        problems.append(f"{step}: {right} switched to workspace {index}")
    if screen != left:
        problems.append(f"{step}: KWin's active output moved to {screen}")


def spill_b_onto(left, right, problems):
    screens = outputs()
    move(screens[right][0] + 400, 540, settle=False)
    open_client("C", client="colored-client.qml")
    wait_for(lambda: settled("C") and active_title() == "C", 60)
    move(screens[left][0] + 400, 540, settle=False)
    for title in ("A", "B"):
        open_client(title, client="colored-client.qml")
        wait_for(lambda: settled(title) and active_title() == title, 60)
    konveyor_action("focus-column-left")
    wait_for(lambda: engine_focus() == "A" and all_settled(), 60)
    konveyor_action("set-column-width", "90%")
    wait_for(lambda: all_settled() and frames()["A"][2] > screens[left][1] * 0.8, 60)
    b = frames()["B"]
    print(f"setup: A={frames()['A']} B={b}")
    if not b[0] < screens[right][0] < b[0] + b[2] / 2:
        problems.append(f"setup: B at {b} does not spill mostly onto {right}")
    for action in ("focus-monitor-right", "focus-workspace-down", "focus-monitor-left"):
        konveyor_action(action)
    wait_for(lambda: active_index(right) == 2 and engine_focus() == "A" and active_output() == left, 60)
    print(f"setup: {right} on workspace {active_index(right)}, focus {engine_focus()} on {active_output()}")
    if active_index(right) != 2:
        problems.append(f"setup: {right} stayed on workspace {active_index(right)}")


def main():
    problems = []
    screens = outputs()
    if len(screens) != 2:
        print(f"expected two outputs, found {len(screens)}")
        print("RESULT: FAIL")
        return
    left, right = sorted(screens, key=lambda name: screens[name][0])
    spill_b_onto(left, right, problems)
    check_owned(f"{right} switched to workspace 2", left, problems)
    check_right_untouched(f"{right} switched to workspace 2", left, right, problems)
    konveyor_action("focus-column-right")
    check_right_untouched("focus-column-right onto B", left, right, problems)
    konveyor_action("focus-column-left")
    check_owned("focus back on A", left, problems)
    for _ in range(3):
        konveyor_action("focus-column-right")
        konveyor_action("focus-column-left")
    check_right_untouched("focus flipped between A and B while animating", left, right, problems)
    check_owned("after the flips", left, problems)
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()


