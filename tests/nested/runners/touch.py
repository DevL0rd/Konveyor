#!/usr/bin/env python3
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from fakepointer import Held, touch
from kwinsession import active_title, konveyor_action, konveyor_windows, qdbus, run_script, wait_for
from screenshot import capture_workspace


def konveyor(method):
    return json.loads(qdbus("org.kde.Konveyor", "/Konveyor", f"org.kde.Konveyor.{method}"))


def columns():
    placed = []
    for window in konveyor_windows():
        layout = window.get("layout") or {}
        position = layout.get("tile_pos_in_workspace_view")
        if position:
            placed.append((round(position[0]), window["title"]))
    return sorted(placed)


def active_workspace():
    return next(workspace["idx"] for workspace in konveyor("Workspaces") if workspace["is_active"])


def overview_open():
    return konveyor("OverviewState")["is_open"]


def title_bar_y(title):
    printed = run_script(f'for (const w of workspace.windowList()) {{ if (w.caption == "{title}") {{ print("MARK|" + w.frameGeometry.y + "|" + w.clientGeometry.y + "|" + w.frameGeometry.x + "|" + w.frameGeometry.width); }} }}')
    frame_y, client_y, frame_x, width = (float(value) for value in printed[0].split("|"))
    return frame_y, client_y, frame_x, width


def settled():
    shown = {}
    for line in run_script('for (const w of workspace.windowList()) { if (!w.deleted && w.normalWindow) { print("MARK|" + w.caption + "|" + w.frameGeometry.x + "|" + w.frameGeometry.width); } }'):
        title, x, width = line.split("|")
        shown[title] = (float(x), float(width))
    active = next(workspace["id"] for workspace in konveyor("Workspaces") if workspace["is_active"])
    for window in konveyor_windows():
        target = (window["layout"]["tile_pos_in_workspace_view"][0], window["layout"]["tile_size"][0])
        frame = shown.get(window["title"])
        if window["workspace_id"] == active and (not frame or abs(frame[0] - target[0]) > 1 or abs(frame[1] - target[1]) > 1):
            return False
    return True


def focused_title():
    return next((window["title"] for window in konveyor_windows() if window["is_focused"]), None)


def settle():
    wait_for(lambda: settled() and active_title() == focused_title(), 60)


def set_widths(width):
    konveyor_action("focus-column-first")
    for _ in range(3):
        konveyor_action("set-column-width", width)
        konveyor_action("focus-column-right")
    konveyor_action("focus-column-first")
    settle()


def check_swipe(problems):
    set_widths("80%")
    before = columns()
    touch(3, 1300, 540, dx=-900, radius=50, steps=40, settle=False)
    wait_for(lambda: columns()[0][0] < before[0][0] - 100, 60)
    after = columns()
    print(f"3-finger swipe left: {before} -> {after}")
    if after[0][0] >= before[0][0] - 100:
        problems.append(f"3-finger swipe did not scroll the row ({before} -> {after})")
    settle()


def check_workspace_swipe(problems):
    start = active_workspace()
    touch(3, 960, 800, dy=-600, radius=50, steps=40, settle=False)
    wait_for(lambda: active_workspace() != start, 60)
    moved = active_workspace()
    touch(3, 960, 200, dy=600, radius=50, steps=40, settle=False)
    wait_for(lambda: active_workspace() == start, 60)
    back = active_workspace()
    print(f"3-finger swipe up: workspace {start} -> {moved}, swipe down -> {back}")
    if moved == start:
        problems.append("3-finger swipe up did not switch workspace")
    if back != start:
        problems.append(f"3-finger swipe down did not come back to workspace {start} (at {back})")
    settle()


def kde_active_effects():
    return qdbus("org.kde.KWin", "/Effects", "org.freedesktop.DBus.Properties.Get", "org.kde.kwin.Effects", "activeEffects").split()


def check_pinch(problems):
    report = Path(os.environ["KONVEYOR_REPORT"])
    before = capture_workspace(str(report.with_suffix(".before-pinch.png")))

    def changed():
        shown = capture_workspace(str(report.with_suffix(".overview.png")))
        return sum(1 for a, b in zip(before.getdata(), shown.getdata()) if a != b) / (before.width * before.height)

    touch(4, 960, 540, radius=300, end_radius=60, steps=30, settle=False)
    wait_for(lambda: overview_open() and "overview" in kde_active_effects() and changed() >= 0.2, 60)
    opened = overview_open()
    effects_open = kde_active_effects()
    fraction = changed()
    touch(4, 960, 540, radius=60, end_radius=300, steps=30, settle=False)
    wait_for(lambda: not overview_open() and "overview" not in kde_active_effects(), 60)
    closed = overview_open()
    effects_closed = kde_active_effects()
    print(f"4-finger pinch in: overview open={opened}, KDE effects {effects_open}, {fraction:.0%} of the screen changed; pinch out: open={closed}, KDE effects {effects_closed}")
    if not opened or "overview" not in effects_open:
        problems.append("4-finger pinch in did not open KDE's Overview")
    if fraction < 0.2:
        problems.append(f"the Overview did not show on screen ({fraction:.0%} of pixels changed)")
    if closed or "overview" in effects_closed:
        problems.append("4-finger pinch out did not close KDE's Overview")
    settle()


def window_place(title):
    window = next(window for window in konveyor_windows() if window["title"] == title)
    return window["workspace_id"], tuple(window["layout"]["pos_in_scrolling_layout"])


def next_place(title, previous):
    wait_for(lambda: window_place(title) != previous, 60)
    return window_place(title)


def window_center(title):
    settle()
    frame_y, _, frame_x, width = title_bar_y(title)
    return frame_x + width / 2, frame_y + 300


def check_window_swipes(problems):
    set_widths("30%")
    konveyor_action("focus-column-right")
    settle()
    title = active_title()
    before = window_place(title)
    x, y = window_center(title)
    touch(4, x + 200, y, dx=-400, radius=60, steps=30, settle=False)
    merged = next_place(title, before)
    x, y = window_center(title)
    touch(4, x - 200, y, dx=400, radius=60, steps=30, settle=False)
    popped = next_place(title, merged)
    x, y = window_center(title)
    touch(4, x, y, dy=400, radius=60, steps=30, settle=False)
    carried = next_place(title, popped)
    print(f"4-finger swipes on {title}: start {before}, left {merged}, right {popped}, down {carried}")
    if merged[1][0] != before[1][0] - 1 or merged[1][1] < 2:
        problems.append(f"4-finger swipe left did not merge {title} into the column on its left ({before} -> {merged})")
    if popped[1][1] != 1 or popped[1][0] != before[1][0]:
        problems.append(f"4-finger swipe right did not pop {title} back out ({merged} -> {popped})")
    if carried[0] == popped[0]:
        problems.append(f"4-finger swipe down did not carry {title} to another workspace ({popped} -> {carried})")
    konveyor_action("move-window-to-workspace-up")
    settle()


def check_tap(problems):
    konveyor_action("focus-column-first")
    settle()
    visible = [(x, title) for x, title in columns() if 0 <= x < 1700]
    target_x, target = visible[1]
    touch(1, target_x + 100, 600, steps=1, settle=False)
    wait_for(lambda: active_title() == target, 60)
    print(f"tap on {target}: KWin active window is {active_title()}")
    if active_title() != target:
        problems.append(f"tapping {target} did not focus it (active: {active_title()})")


def tile_width(title):
    window = next(window for window in konveyor_windows() if window["title"] == title)
    return window["layout"]["tile_size"][0]


def check_three_finger_tap(problems):
    set_widths("30%")
    visible = [(x, title) for x, title in columns() if 0 <= x < 1700]
    target_x, target = visible[1]
    before = tile_width(target)
    touch(3, target_x + before / 2, 600, radius=60, steps=1, settle=False)
    wait_for(lambda: active_title() == target and abs(tile_width(target) - before) >= 1, 60)
    after = tile_width(target)
    print(f"3-finger tap on {target}: KWin active window is {active_title()}, width {before} -> {after}")
    if active_title() != target:
        problems.append(f"a 3-finger tap on {target} did not focus it (active: {active_title()})")
    if abs(after - before) < 1:
        problems.append(f"a 3-finger tap on {target} did not change its width ({before} -> {after})")


def check_long_press(problems):
    set_widths("30%")
    before = [title for _, title in columns()]
    first = before[0]
    frame_y, client_y, frame_x, width = title_bar_y(first)
    if client_y - frame_y < 10:
        problems.append(f"{first} has no title bar to hold (frame y {frame_y}, client y {client_y})")
        return
    touch(1, frame_x + width / 3, (frame_y + client_y) / 2, dx=800, hold_ms=800, steps=40, settle=False)
    wait_for(lambda: [title for _, title in columns()].index(first) != 0, 60)
    placed = columns()
    after = [title for _, title in placed]
    print(f"long press {first}'s title bar and drag right: {before} -> {placed}")
    if after.index(first) == 0:
        problems.append(f"long-press drag did not move {first} ({before} -> {after})")


def check_quick_drag_scrolls(problems):
    set_widths("45%")
    before = columns()
    second = before[1][1]
    frame_y, client_y, frame_x, width = title_bar_y(second)
    touch(1, frame_x + width * 0.8, (frame_y + client_y) / 2, dx=-700, steps=30, settle=False)
    wait_for(lambda: columns()[0][0] < before[0][0] - 100, 60)
    after = columns()
    print(f"quick drag on {second}'s title bar: {before} -> {after}")
    if [title for _, title in after] != [title for _, title in before]:
        problems.append(f"a quick title bar drag reordered the row instead of scrolling it ({before} -> {after})")
    if after[0][0] >= before[0][0] - 100:
        problems.append(f"a quick title bar drag did not scroll the row ({before} -> {after})")


def check_floating_drag(problems):
    konveyor_action("focus-column-first")
    konveyor_action("toggle-window-floating")
    settle()
    title = active_title()
    frame_y, client_y, frame_x, width = title_bar_y(title)
    touch(1, frame_x + width / 2, (frame_y + client_y) / 2, dx=300, dy=150, steps=30, settle=False)

    def moved_by(dx, dy):
        moved_y, _, moved_x, _ = title_bar_y(title)
        return abs(moved_x - frame_x - dx) <= 40 and abs(moved_y - frame_y - dy) <= 40

    wait_for(lambda: moved_by(300, 150), 60)
    moved_y, _, moved_x, _ = title_bar_y(title)
    print(f"drag floating {title}: ({frame_x}, {frame_y}) -> ({moved_x}, {moved_y})")
    if not moved_by(300, 150):
        problems.append(f"dragging floating {title} by (300, 150) moved it to ({moved_x}, {moved_y}) from ({frame_x}, {frame_y})")
    konveyor_action("toggle-window-floating")
    settle()


def multi_touch_active():
    return qdbus("org.kde.Konveyor", "/Konveyor", "org.kde.Konveyor.MultiTouchActive") == "true"


def check_cancelled_touch(problems):
    set_widths("80%")
    before = columns()
    with Held() as held:
        held.send(*(f"touchdown:{finger}:{1300 + finger * 60}:540" for finger in range(3)))
        active = wait_for(multi_touch_active, 30)
        held.send(*(f"touchmotion:{finger}:{1000 + finger * 60}:540" for finger in range(3)))
        held.send("touchcancel")
    released = wait_for(lambda: not multi_touch_active(), 30)
    settle()
    after = columns()
    print(f"cancelled 3-finger touch: multi-touch while down {active}, after the cancel {not released}, row {before} -> {after}")
    if not active:
        problems.append("three fingers down did not report multi-touch")
    if not released:
        problems.append("a cancelled touch left MultiTouchActive on")
    touch(3, 600, 540, dx=900, radius=50, steps=40, settle=False)
    if not wait_for(lambda: columns()[0][0] > after[0][0] + 100, 60):
        problems.append(f"a 3-finger swipe after a cancelled touch did not scroll the row back ({after} -> {columns()})")
    settle()


def main():
    problems = []
    for check in (check_swipe, check_workspace_swipe, check_pinch, check_window_swipes, check_tap, check_three_finger_tap, check_quick_drag_scrolls, check_long_press, check_floating_drag, check_cancelled_touch):
        check(problems)
    capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).with_suffix(".png")))
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
