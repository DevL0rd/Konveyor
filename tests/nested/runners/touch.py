#!/usr/bin/env python3
import json
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from fakepointer import touch
from kwinsession import active_title, konveyor_action, konveyor_windows, qdbus, run_script
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


def set_widths(width):
    konveyor_action("focus-column-first")
    for _ in range(3):
        konveyor_action("set-column-width", width)
        konveyor_action("focus-column-right")
    konveyor_action("focus-column-first")
    time.sleep(1.5)


def check_swipe(problems):
    set_widths("80%")
    before = columns()
    touch(3, 1300, 540, dx=-900, radius=50, steps=40)
    time.sleep(1.0)
    after = columns()
    print(f"3-finger swipe left: {before} -> {after}")
    if after[0][0] >= before[0][0] - 100:
        problems.append(f"3-finger swipe did not scroll the row ({before} -> {after})")


def check_workspace_swipe(problems):
    start = active_workspace()
    touch(3, 960, 800, dy=-600, radius=50, steps=40)
    time.sleep(1.0)
    moved = active_workspace()
    touch(3, 960, 200, dy=600, radius=50, steps=40)
    time.sleep(1.0)
    back = active_workspace()
    print(f"3-finger swipe up: workspace {start} -> {moved}, swipe down -> {back}")
    if moved == start:
        problems.append("3-finger swipe up did not switch workspace")
    if back != start:
        problems.append(f"3-finger swipe down did not come back to workspace {start} (at {back})")


def kde_active_effects():
    return qdbus("org.kde.KWin", "/Effects", "org.freedesktop.DBus.Properties.Get", "org.kde.kwin.Effects", "activeEffects").split()


def check_pinch(problems):
    report = Path(os.environ["KONVEYOR_REPORT"])
    before = capture_workspace(str(report.with_suffix(".before-pinch.png")))
    touch(4, 960, 540, radius=300, end_radius=60, steps=30)
    time.sleep(1.0)
    opened = overview_open()
    effects_open = kde_active_effects()
    shown = capture_workspace(str(report.with_suffix(".overview.png")))
    changed = sum(1 for a, b in zip(before.getdata(), shown.getdata()) if a != b) / (before.width * before.height)
    touch(4, 960, 540, radius=60, end_radius=300, steps=30)
    time.sleep(1.0)
    closed = overview_open()
    effects_closed = kde_active_effects()
    print(f"4-finger pinch in: overview open={opened}, KDE effects {effects_open}, {changed:.0%} of the screen changed; pinch out: open={closed}, KDE effects {effects_closed}")
    if not opened or "overview" not in effects_open:
        problems.append("4-finger pinch in did not open KDE's Overview")
    if changed < 0.2:
        problems.append(f"the Overview did not show on screen ({changed:.0%} of pixels changed)")
    if closed or "overview" in effects_closed:
        problems.append("4-finger pinch out did not close KDE's Overview")


def window_place(title):
    window = next(window for window in konveyor_windows() if window["title"] == title)
    return window["workspace_id"], tuple(window["layout"]["pos_in_scrolling_layout"])


def window_center(title):
    frame_y, _, frame_x, width = title_bar_y(title)
    return frame_x + width / 2, frame_y + 300


def check_window_swipes(problems):
    set_widths("30%")
    konveyor_action("focus-column-right")
    time.sleep(1.0)
    title = active_title()
    before = window_place(title)
    x, y = window_center(title)
    touch(4, x + 200, y, dx=-400, radius=60, steps=30)
    merged = window_place(title)
    x, y = window_center(title)
    touch(4, x - 200, y, dx=400, radius=60, steps=30)
    popped = window_place(title)
    x, y = window_center(title)
    touch(4, x, y, dy=400, radius=60, steps=30)
    carried = window_place(title)
    print(f"4-finger swipes on {title}: start {before}, left {merged}, right {popped}, down {carried}")
    if merged[1][0] != before[1][0] - 1 or merged[1][1] < 2:
        problems.append(f"4-finger swipe left did not merge {title} into the column on its left ({before} -> {merged})")
    if popped[1][1] != 1 or popped[1][0] != before[1][0]:
        problems.append(f"4-finger swipe right did not pop {title} back out ({merged} -> {popped})")
    if carried[0] == popped[0]:
        problems.append(f"4-finger swipe down did not carry {title} to another workspace ({popped} -> {carried})")
    konveyor_action("move-window-to-workspace-up")
    time.sleep(1.5)


def check_tap(problems):
    konveyor_action("focus-column-first")
    time.sleep(1.5)
    visible = [(x, title) for x, title in columns() if 0 <= x < 1700]
    target_x, target = visible[1]
    touch(1, target_x + 100, 600, steps=1)
    print(f"tap on {target}: KWin active window is {active_title()}")
    if active_title() != target:
        problems.append(f"tapping {target} did not focus it (active: {active_title()})")


def check_long_press(problems):
    set_widths("30%")
    before = [title for _, title in columns()]
    first = before[0]
    frame_y, client_y, frame_x, width = title_bar_y(first)
    if client_y - frame_y < 10:
        problems.append(f"{first} has no title bar to hold (frame y {frame_y}, client y {client_y})")
        return
    touch(1, frame_x + width / 3, (frame_y + client_y) / 2, dx=800, hold_ms=800, steps=40)
    time.sleep(2.0)
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
    touch(1, frame_x + width * 0.8, (frame_y + client_y) / 2, dx=-700, steps=30)
    time.sleep(1.5)
    after = columns()
    print(f"quick drag on {second}'s title bar: {before} -> {after}")
    if [title for _, title in after] != [title for _, title in before]:
        problems.append(f"a quick title bar drag reordered the row instead of scrolling it ({before} -> {after})")
    if after[0][0] >= before[0][0] - 100:
        problems.append(f"a quick title bar drag did not scroll the row ({before} -> {after})")


def check_floating_drag(problems):
    konveyor_action("focus-column-first")
    konveyor_action("toggle-window-floating")
    time.sleep(1.5)
    title = active_title()
    frame_y, client_y, frame_x, width = title_bar_y(title)
    touch(1, frame_x + width / 2, (frame_y + client_y) / 2, dx=300, dy=150, steps=30)
    time.sleep(1.0)
    moved_y, _, moved_x, _ = title_bar_y(title)
    print(f"drag floating {title}: ({frame_x}, {frame_y}) -> ({moved_x}, {moved_y})")
    if abs(moved_x - frame_x - 300) > 40 or abs(moved_y - frame_y - 150) > 40:
        problems.append(f"dragging floating {title} by (300, 150) moved it to ({moved_x}, {moved_y}) from ({frame_x}, {frame_y})")
    konveyor_action("toggle-window-floating")
    time.sleep(1.0)


def main():
    problems = []
    for check in (check_swipe, check_workspace_swipe, check_pinch, check_window_swipes, check_tap, check_quick_drag_scrolls, check_long_press, check_floating_drag):
        check(problems)
    capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).with_suffix(".png")))
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
