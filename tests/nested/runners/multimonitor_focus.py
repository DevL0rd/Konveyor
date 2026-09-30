#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from fakepointer import move
from kwinsession import activate, active_title, for_window, konveyor, konveyor_action, konveyor_windows, open_client, run_script, wait_for


def outputs():
    printed = run_script('for (const o of workspace.screens) { print("MARK|" + o.name + "|" + o.geometry.x + "|" + o.geometry.width); }')
    result = {}
    for line in printed:
        name, x, width = line.split("|")
        result[name] = (float(x), float(width))
    return result


def output_at(x):
    return next((name for name, (left, width) in outputs().items() if left <= x < left + width), None)


def frame_center(title):
    printed = run_script(f'for (const w of workspace.windowList()) {{ if (!w.deleted && w.caption == "{title}") {{ const g = w.frameGeometry; print("MARK|" + (g.x + g.width / 2)); }} }}')
    return float(printed[0]) if printed else None


def active_output():
    printed = run_script('print("MARK|" + workspace.activeScreen.name);')
    return printed[0] if printed else None


def engine_focus():
    return next((window.get("title") for window in konveyor_windows() if window.get("is_focused")), None)


def placed(title):
    window = next((window for window in konveyor_windows() if window["title"] == title), None)
    frame = run_script(for_window(title, 'print("MARK|" + w.frameGeometry.x + "|" + w.frameGeometry.width);'))
    if not window or not frame:
        return False
    x, width = (float(value) for value in frame[0].split("|"))
    return abs(x - window["layout"]["tile_pos_in_workspace_view"][0]) < 1 and abs(width - window["layout"]["tile_size"][0]) < 1


def perform(name, *arguments):
    konveyor_action(name, *arguments)
    wait_for(lambda: placed(engine_focus()), 60)


def engine_output():
    focused = json.loads(konveyor("FocusedOutput"))
    return focused[0]["name"] if focused else None


def focus(title):
    activate(title)
    if not wait_for(lambda: engine_focus() == title and placed(title) and engine_output() == output_at(frame_center(title)), 60):
        print(f"{title} activated, but the layout focused {engine_focus()} on {engine_output()}")


def check_focus(step, expected, expected_output, problems):
    wait_for(lambda: active_title() == engine_focus() == expected and active_output() == expected_output, 60)
    kwin, engine, screen = active_title(), engine_focus(), active_output()
    print(f"{step}: kwin={kwin} engine={engine} active output={screen}")
    if kwin != expected or engine != expected:
        problems.append(f"{step}: expected {expected} focused, KWin activated {kwin} and the layout focused {engine}")
    if screen != expected_output:
        problems.append(f"{step}: KWin's active output is {screen}, not {expected_output}")


def cross_edge(problems, secondary, primary, far, near, direction, back):
    focus(far)
    center = frame_center(near)
    spill = output_at(center)
    print(f"focused {far}; {near} is centered at x={center} on {spill}")
    if spill != secondary:
        problems.append(f"setup: {near} is centered on {spill}, the edge case needs it on {secondary}")
        return
    perform(f"focus-column-{direction}")
    check_focus(f"focus-column-{direction} from {far}", near, primary, problems)
    wait_for(lambda: placed(near), 60)
    check_focus(f"{near} once it settled", near, primary, problems)
    perform(f"focus-column-{back}")
    check_focus(f"focus-column-{back} back to {far}", far, primary, problems)


def main():
    problems = []
    names = sorted(outputs(), key=lambda name: outputs()[name][0])
    if len(names) < 2:
        print("expected two outputs")
        print("RESULT: FAIL")
        return
    left, right = names[0], names[1]
    move(outputs()[left][0] + 400, 540, settle=False)
    for title in ("A", "B", "C"):
        open_client(title)
        wait_for(lambda: placed(title) and active_title() == title, 60)
        if output_at(frame_center(title)) != left:
            problems.append(f"setup: {title} opened on {output_at(frame_center(title))} with the pointer on {left}")
    for title in ("B", "C"):
        focus(title)
        perform("move-column-to-monitor-right")
    for title in ("B", "C"):
        focus(title)
        perform("set-column-width", "75%")
    print(f"layout: {[(w['title'], w['layout'].get('pos_in_scrolling_layout')) for w in konveyor_windows()]}")
    cross_edge(problems, left, right, "C", "B", "left", "right")
    for title in ("B", "C"):
        focus(title)
        perform("move-column-to-monitor-left")
    focus("A")
    perform("move-column-to-monitor-right")
    for title in ("B", "C"):
        focus(title)
        perform("set-column-width", "75%")
    focus("C")
    perform("move-column-to-first")
    cross_edge(problems, right, left, "C", "B", "right", "left")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
