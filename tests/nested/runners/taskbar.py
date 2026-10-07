#!/usr/bin/env python3
import json
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from checks import Checks
from fakepointer import Held, click
from kwinsession import active_title, column_titles, konveyor, konveyor_action, wait_for, window_minimized
from screenshot import capture_workspace
from widgets_load import install, load_errors, plasma

ROOT = Path(os.environ["KONVEYOR_TEST_ROOT"])
PANEL = """
var panel = new Panel("org.kde.panel");
panel.location = "bottom";
panel.height = 48;
var taskbar = panel.addWidget("org.devl0rd.taskbar");
taskbar.currentConfigGroup = ["General"];
taskbar.writeConfig("groupMode", 2);
"""


def panel_pixels(matches):
    image = capture_workspace(ROOT / "taskbar.png").convert("RGB")
    width, height = image.size
    pixels = image.load()
    return [(x, y) for y in range(height - 48, height) for x in range(width) if matches(pixels[x, y])]


def runs(hits):
    found, run = [], []
    for x in sorted({x for x, _ in hits}):
        if run and x - run[-1] > 4:
            found.append(run)
            run = []
        run.append(x)
    return found + [run] if run else found


def icon_centers():
    hits = panel_pixels(lambda pixel: pixel[1] > 140 and pixel[1] > pixel[0] + 50 and pixel[1] > pixel[2] + 50)
    y = round(sum(y for _, y in hits) / len(hits)) if hits else 0
    return [((run[0] + run[-1]) // 2, y) for run in runs(hits)]


def current_pill():
    icons = icon_centers()
    if not icons:
        return None
    first_icon, row = icons[0]
    hits = [(x, y) for x, y in panel_pixels(lambda pixel: pixel[2] > 180 and pixel[0] < 120 and 130 < pixel[1] < 210)
            if 16 < x < first_icon - 24 and abs(y - row) <= 4]
    if not hits:
        return None
    return min(x for x, _ in hits), max(x for x, _ in hits), round(sum(y for _, y in hits) / len(hits))


def focused_index():
    return next(workspace["idx"] for workspace in json.loads(konveyor("Workspaces")) if workspace["is_focused"])


def three_icons():
    centers = icon_centers()
    return centers if len(centers) == 3 else None


def ids():
    return {window["title"]: window["id"] for window in json.loads(konveyor("Windows"))}


def panel_loads(checks):
    install(checks)
    checks.expect(wait_for(lambda: plasma("print(desktops().length);") not in ("", "0"), 180, 1), "plasmashell is up with a desktop")
    plasma(PANEL)
    checks.expect(wait_for(three_icons, 60, 1), f"the taskbar shows one icon per column ({icon_centers()})")
    checks.expect(wait_for(lambda: three_icons() == three_icons(), 20, 0.5), "the panel settles")
    checks.equal(load_errors("org.devl0rd.taskbar"), [], "the taskbar loads in a panel without QML errors")


def icons_follow_columns(checks):
    icons = three_icons()
    order = column_titles()
    for index, (x, y) in enumerate(icons):
        click(x, y)
        checks.expect(wait_for(lambda: active_title() == order[index], 10), f"icon {index + 1} activates column {index + 1} ({order[index]})")


def konveyor_moves_reorder_icons(checks):
    before = column_titles()
    konveyor("Action", json.dumps({"name": "move-column-to-index", "arguments": ["3"], "properties": {}, "id": ids()[before[0]]}))
    checks.expect(wait_for(lambda: column_titles() == [before[1], before[2], before[0]]), "Konveyor moves the column")
    time.sleep(1)
    x, y = three_icons()[2]
    click(x, y)
    checks.expect(wait_for(lambda: active_title() == before[0], 10), f"the last icon now belongs to the moved column ({active_title()})")
    x, y = three_icons()[0]
    click(x, y)
    checks.expect(wait_for(lambda: active_title() == before[1], 10), f"the first icon follows ({active_title()})")


def dragging_icons_moves_columns(checks):
    before = column_titles()
    focused = active_title()
    icons = three_icons()
    (start_x, y), (end_x, _) = icons[2], icons[0]
    with Held() as pointer:
        pointer.send(f"move:{start_x}:{y}")
        time.sleep(0.3)
        pointer.send("button:272:1")
        for step in range(1, 17):
            pointer.send(f"move:{start_x + (end_x - start_x) * step / 16}:{y}")
            time.sleep(0.03)
        time.sleep(0.3)
        pointer.send("button:272:0")
    expected = [before[2], before[0], before[1]]
    checks.expect(wait_for(lambda: column_titles() == expected, 10), f"dragging the last icon first moves its column first ({column_titles()})")
    checks.equal(active_title(), focused, "dragging an icon leaves focus alone")


def clicking_the_active_icon_minimizes(checks):
    time.sleep(1)
    order = column_titles()
    x, y = three_icons()[0]
    click(x, y)
    checks.expect(wait_for(lambda: active_title() == order[0], 10), "clicking an icon activates its window")
    click(x, y)
    checks.expect(wait_for(lambda: window_minimized(order[0]), 10), "clicking the active icon minimizes it")
    click(x, y)
    checks.expect(wait_for(lambda: not window_minimized(order[0]) and active_title() == order[0], 10), "clicking it again restores it")


def workspaces_switch(checks):
    checks.equal(konveyor_action("focus-workspace", "2"), "", "switching to the empty workspace")
    checks.expect(wait_for(lambda: icon_centers() == [], 10), "the taskbar shows only the current workspace's columns")
    checks.equal(konveyor_action("focus-workspace", "1"), "", "switching back")
    checks.expect(wait_for(three_icons, 10), "the icons come back")


def workspace_pills_switch(checks):
    left, right, y = current_pill()
    click(right + 18, y)
    checks.expect(wait_for(lambda: focused_index() == 2, 10), f"clicking the next workspace pill switches to it ({focused_index()})")
    checks.expect(wait_for(lambda: icon_centers() == [], 10), "the icons follow the workspace")
    with Held() as pointer:
        pointer.send(f"move:{left}:{y}")
        time.sleep(0.3)
        pointer.send("axis:0:-15")
    checks.expect(wait_for(lambda: focused_index() == 1, 10), f"scrolling up over the workspaces goes back ({focused_index()})")
    checks.expect(wait_for(three_icons, 10), "and the icons come back")


def middle_click_closes(checks):
    order = column_titles()
    x, y = three_icons()[1]
    with Held() as pointer:
        pointer.send(f"move:{x}:{y}")
        time.sleep(0.3)
        pointer.send("button:274:1")
        time.sleep(0.1)
        pointer.send("button:274:0")
    checks.expect(wait_for(lambda: column_titles() == [order[0], order[2]], 10), f"middle-clicking an icon closes its window ({column_titles()})")


def main():
    Checks().run(panel_loads, icons_follow_columns, konveyor_moves_reorder_icons, dragging_icons_moves_columns,
                 clicking_the_active_icon_minimizes, workspaces_switch, workspace_pills_switch, middle_click_closes)


if __name__ == "__main__":
    main()
