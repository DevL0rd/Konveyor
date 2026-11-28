#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from dragging import dragging, settled, window_json
from kwinsession import activate, for_window, frame, konveyor, konveyor_action, run_script, wait_for


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


def engine_output(title):
    window = window_json(title)
    return next((workspace["output"] for workspace in json.loads(konveyor("Workspaces")) if workspace["id"] == window["workspace_id"]), None)


def describe(title):
    return f"frame={frame(title)} engine output={engine_output(title)} KWin output={kwin_output(title)} floating={window_json(title)['is_floating']}"


def settled_on(title, output, screens):
    left, width = screens[output]

    def there():
        current = frame(title)
        return current and left <= current[0] + current[2] / 2 < left + width and engine_output(title) == output and kwin_output(title) == output

    return wait_for(there, 10)


def outputs_of(title):
    return engine_output(title)


def float_window(checks, title):
    activate(title)
    checks.equal(konveyor_action("toggle-window-floating"), "", f"float {title}")
    checks.expect(wait_for(lambda: window_json(title)["is_floating"], 10), f"{title} is floating")
    settled(title)


def drag_floating(checks, title, goal, screens):
    activate(title)
    left, width = screens[outputs_of(title)]
    x, y, w, h = frame(title)
    grab = (round(max(x, left) + min(w, 300) / 2), round(y + 20))
    with dragging(title, goal, grab=grab):
        pass
    settled(title)


def dropped_fully_on_other_monitor(checks, screens, left, right):
    float_window(checks, "A")
    checks.equal(engine_output("A"), left, "A starts on the left monitor")
    drag_floating(checks, "A", (screens[right][0] + 700, 500), screens)
    checks.expect(settled_on("A", right, screens), f"A dropped fully on the right monitor stays there and belongs to it ({describe('A')})")


def dragged_back_to_first(checks, screens, left, right):
    drag_floating(checks, "A", (screens[left][0] + 700, 500), screens)
    checks.expect(settled_on("A", left, screens), f"A dragged back belongs to the left monitor again ({describe('A')})")


def dropped_straddling(checks, screens, left, right):
    boundary = screens[right][0]
    drag_floating(checks, "A", (round(boundary - 350), 500), screens)
    x, y, w, h = frame("A")
    checks.expect(x < boundary < x + w, f"A straddles the edge ({describe('A')})")
    checks.expect(settled_on("A", left, screens), f"A mostly on the left monitor stays on it and keeps its straddling frame ({describe('A')})")
    drag_floating(checks, "A", (round(boundary + 100), 500), screens)
    x, y, w, h = frame("A")
    checks.expect(x < boundary < x + w, f"A straddles the edge again ({describe('A')})")
    checks.expect(settled_on("A", right, screens), f"A mostly on the right monitor belongs to it and keeps its straddling frame ({describe('A')})")


def main():
    screens = outputs()
    checks = Checks()
    if len(screens) != 2:
        print(f"expected two outputs, found {len(screens)}")
        print("RESULT: FAIL")
        return
    left, right = sorted(screens, key=lambda name: screens[name][0])

    def fully_on_other_monitor(checks):
        dropped_fully_on_other_monitor(checks, screens, left, right)

    def dragged_back(checks):
        dragged_back_to_first(checks, screens, left, right)

    def straddling(checks):
        dropped_straddling(checks, screens, left, right)

    checks.run(fully_on_other_monitor, dragged_back, straddling)


if __name__ == "__main__":
    main()
