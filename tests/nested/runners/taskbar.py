#!/usr/bin/env python3
import json
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from checks import Checks
from fakepointer import Held, click, keys
from checks import config_path
from kwinsession import (activate, active_title, column_titles, konveyor, konveyor_action, konveyor_windows, run_script, wait_for,
                         window_minimized)
from screenshot import capture_workspace
from widgets_load import install, load_errors, plasma

ROOT = Path(os.environ["KONVEYOR_TEST_ROOT"])
PANEL = """
var panel = new Panel("org.kde.panel");
panel.location = "bottom";
panel.height = 48;
panel.addWidget("org.devl0rd.taskbar");
"""
TASKBARRC = Path(os.environ["XDG_CONFIG_HOME"]) / "konveyor" / "taskbarrc"


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


def columns():
    titles = {window["id"]: window["title"] for window in konveyor_windows()}
    workspace = next(workspace for workspace in json.loads(konveyor("Workspaces")) if workspace["is_focused"])
    return [[titles[id] for id in column] for column in workspace["columns"]]


def act_on(title, name, *arguments):
    return konveyor("Action", json.dumps({"name": name, "arguments": list(arguments), "properties": {}, "id": ids()[title]}))


def widths():
    return {window["title"]: round(window["layout"]["tile_size"][0]) for window in konveyor_windows()}


def strong_blue(pixel):
    return pixel[2] > 180 and pixel[0] < 120 and 130 < pixel[1] < 210


def badge_pixels(icons, pill):
    hits = panel_pixels(strong_blue)
    near_icons = sum(1 for x, y in hits if icons[0][0] - 20 <= x <= icons[-1][0] + 24 and y < icons[0][1] - 8)
    near_pills = sum(1 for x, y in hits if pill[0] - 8 <= x <= pill[1] + 24 and y < pill[2] - 13)
    return near_icons, near_pills


def popups():
    return run_script('for (const w of workspace.windowList()) { if (!w.deleted && w.popupWindow) print("MARK|" + w.caption); }')


def panel_loads(checks):
    install(checks)
    checks.expect(wait_for(lambda: plasma("print(desktops().length);") not in ("", "0"), 180, 1), "plasmashell is up with a desktop")
    TASKBARRC.parent.mkdir(parents=True, exist_ok=True)
    TASKBARRC.write_text("[General]\ngroupMode=2\n")
    plasma(PANEL)
    checks.expect(wait_for(three_icons, 60, 1), f"the taskbar shows one icon per column ({icon_centers()})")
    checks.expect(wait_for(lambda: three_icons() == three_icons(), 20, 0.5), "the panel settles")
    checks.equal(load_errors("org.devl0rd.taskbar"), [], "the taskbar loads in a panel without QML errors")


def icons_follow_columns(checks):
    icons = three_icons()
    order = column_titles()
    activate(order[2])
    wait_for(lambda: active_title() == order[2], 10)
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


def shared_column(checks):
    order = column_titles()
    checks.equal(act_on(order[1], "consume-or-expel-window-left"), "", "the second window joins the first column")
    checks.expect(wait_for(lambda: columns() == [[order[0], order[1]], [order[2]]], 10), f"two windows share a column ({columns()})")
    checks.expect(wait_for(three_icons, 10), f"each window of the shared column keeps its own icon ({icon_centers()})")
    activate(order[2])
    wait_for(lambda: active_title() == order[2], 10)
    time.sleep(1)
    for index, title in enumerate(order):
        x, y = three_icons()[index]
        click(x, y)
        checks.expect(wait_for(lambda: active_title() == title, 10), f"icon {index + 1} focuses {title} ({active_title()})")
    checks.equal(act_on(order[1], "consume-or-expel-window-right"), "", "the window moves back out to its own column")
    checks.expect(wait_for(lambda: len(columns()) == 3, 10), f"three columns again ({columns()})")
    checks.expect(wait_for(three_icons, 10), "and three icons")


def shortcut_badges(checks):
    time.sleep(1)
    icons, pill = three_icons(), current_pill()
    before = badge_pixels(icons, pill)
    with Held() as pointer:
        pointer.send((125, 1))
        time.sleep(1)
        meta = badge_pixels(icons, pill)
        pointer.send((56, 1))
        time.sleep(1)
        both = badge_pixels(icons, pill)
        pointer.send((56, 0), (125, 0))
    checks.expect(meta[1] > before[1] + 20 and meta[0] <= before[0] + 5, f"Meta shows the workspace numbers only ({before} -> {meta})")
    checks.expect(both[0] > before[0] + 20 and both[1] <= before[1] + 5, f"Meta+Alt shows the app numbers instead ({before} -> {both})")
    checks.expect(wait_for(lambda: badge_pixels(icons, pill)[0] <= before[0] + 5, 5), "the numbers go away when the keys are released")


def menu_actions(checks):
    order = column_titles()
    x, y = three_icons()[0]
    before = len(popups())

    def right_click():
        with Held() as pointer:
            pointer.send(f"move:{x}:{y}")
            time.sleep(0.3)
            pointer.send("button:273:1", "button:273:0")
        return wait_for(lambda: len(popups()) > before, 5)

    checks.expect(right_click() or right_click(), "right-clicking an icon opens its menu")
    keys((1, 1), (1, 0))
    checks.expect(wait_for(lambda: len(popups()) == before, 10), "Escape closes it")
    width = widths()[order[0]]
    checks.equal(act_on(order[0], "set-column-width", "+10%"), "", "Wider, as the menu sends it")
    checks.expect(wait_for(lambda: widths()[order[0]] > width, 10), f"the column gets wider ({width} -> {widths()[order[0]]})")
    checks.equal(act_on(order[2], "consume-or-expel-window-left"), "", "Join the Column on the Left, as the menu sends it")
    checks.expect(wait_for(lambda: columns() == [[order[0]], [order[1], order[2]]], 10), f"the window joins the column ({columns()})")
    rule = {"id": ids()[order[0]], "option": "float", "enabled": True}
    floating = config_path().read_text().count("open-floating true")
    checks.equal(konveyor("SetAppRule", json.dumps(rule)), "", "Always Float, as the menu sends it")
    checks.equal(config_path().read_text().count("open-floating true"), floating + 1, "the rule is written to config.kdl")
    checks.equal(json.loads(konveyor("AppRules", json.dumps({"id": rule["id"]})))["rules"]["float"], True, "and read back for the menu")
    checks.equal(konveyor("SetAppRule", json.dumps({**rule, "enabled": False})), "", "unchecking removes the rule")
    checks.equal(config_path().read_text().count("open-floating true"), floating, "the rule is gone from config.kdl")
    act_on(order[2], "consume-or-expel-window-right")
    checks.expect(wait_for(lambda: len(columns()) == 3, 10), f"three columns again ({columns()})")


def settings_reach_the_widget(checks):
    TASKBARRC.write_text("[General]\ngroupMode=2\nshowApps=false\n")
    checks.expect(wait_for(lambda: icon_centers() == [], 10), "turning Show apps off in taskbarrc hides the icons")
    TASKBARRC.write_text("[General]\ngroupMode=2\n")
    checks.expect(wait_for(three_icons, 10), "and turning it back on shows them again")


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
                 clicking_the_active_icon_minimizes, workspaces_switch, workspace_pills_switch, shared_column, shortcut_badges, menu_actions,
                 settings_reach_the_widget, middle_click_closes)


if __name__ == "__main__":
    main()
