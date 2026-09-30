#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

import virtualtouchpad as touchpad
from checks import Checks, default_config, load_config
from fakepointer import move, tap
from keycodes import KEY_CODES
from kwinsession import active_title, frame, konveyor_action, konveyor_windows, wait_for
from touch import active_workspace, columns, kde_active_effects, konveyor, next_place, overview_open, set_widths, settle, window_place


KEY_ESCAPE = KEY_CODES["Escape"]


def column_size(place):
    return sum(1 for window in konveyor_windows() if window["workspace_id"] == place[0] and window["layout"]["pos_in_scrolling_layout"][0] == place[1][0])


def first_column_x():
    return columns()[0][0]


def row_scrolls(checks):
    set_widths("80%")
    start = first_column_x()
    touchpad.swipe(3, dx=-600)
    checks.expect(wait_for(lambda: first_column_x() < start - 100, 30), f"a 3-finger swipe left scrolls the row ({start} -> {first_column_x()})")
    settle()
    scrolled = first_column_x()
    touchpad.swipe(3, dx=600, cancel=True)
    checks.expect(wait_for(lambda: first_column_x() > scrolled + 100, 30),
                  f"a cancelled 3-finger swipe right still settles the row where it was left ({scrolled} -> {first_column_x()})")
    settle()


def natural_swipe_flips_the_row(checks):
    checks.expect(load_config(default_config().replace("        natural-swipe\n", "        natural-swipe false\n", 1)),
                  "a config without natural-swipe for the touchpad loads")
    settle()
    start = first_column_x()
    touchpad.swipe(3, dx=600)
    checks.expect(wait_for(lambda: first_column_x() < start - 100, 30),
                  f"without natural-swipe a swipe right scrolls the row the other way ({start} -> {first_column_x()})")
    settle()
    load_config(default_config())
    settle()


def workspaces_switch(checks):
    start = active_workspace()
    touchpad.swipe(3, dy=-400)
    checks.expect(wait_for(lambda: active_workspace() != start, 30), f"a 3-finger swipe up leaves workspace {start}")
    touchpad.swipe(3, dy=400)
    checks.expect(wait_for(lambda: active_workspace() == start, 30), f"a 3-finger swipe down comes back to workspace {start}")
    settle()


def pinch_toggles_overview(checks):
    touchpad.pinch(4, 0.4)
    checks.expect(wait_for(lambda: overview_open() and "overview" in kde_active_effects(), 30), "a 4-finger pinch in opens KDE's Overview")
    touchpad.pinch(4, 2.5)
    checks.expect(wait_for(lambda: not overview_open() and "overview" not in kde_active_effects(), 30), "a 4-finger pinch out closes it")
    touchpad.pinch(4, 0.4, cancel=True)
    checks.expect(wait_for(lambda: overview_open(), 30), "a cancelled 4-finger pinch in still opens the Overview")
    touchpad.pinch(4, 2.5)
    checks.expect(wait_for(lambda: not overview_open() and "overview" not in kde_active_effects(), 30), "and a pinch out closes it again")
    settle()


def window_swipes(checks):
    set_widths("30%")
    konveyor_action("focus-column-right")
    settle()
    title = active_title()
    before = window_place(title)
    touchpad.swipe(4, dx=-200)
    merged = next_place(title, before)
    checks.expect(merged[1][0] == before[1][0] - 1 and column_size(merged) == 2,
                  f"a 4-finger swipe left merges {title} into the left column ({before} -> {merged})")
    touchpad.swipe(4, dx=200)
    popped = next_place(title, merged)
    checks.expect(popped[1][1] == 1 and popped[1][0] == before[1][0], f"a 4-finger swipe right pops {title} back out ({merged} -> {popped})")
    touchpad.swipe(4, dy=200)
    carried = next_place(title, popped)
    checks.expect(carried[0] != popped[0], f"a 4-finger swipe down carries {title} to another workspace ({popped} -> {carried})")
    konveyor_action("move-window-to-workspace-up")
    settle()


def other_finger_counts_pass_through(checks):
    before = (columns(), active_workspace(), overview_open())
    touchpad.swipe(5, dx=-600)
    touchpad.swipe(2, dy=-400)
    touchpad.pinch(2, 0.4)
    touchpad.pinch(3, 0.4)
    settle()
    checks.equal((columns(), active_workspace(), overview_open()), before, "swipes and pinches with unconfigured finger counts leave the row alone")


def gestures_off_hand_swipes_to_kwin(checks):
    config = default_config().replace("    touchpad {\n", "    touchpad {\n        off\n", 1)
    checks.expect(load_config(config), "a config with touchpad gestures off loads")
    before = (columns(), active_workspace())
    touchpad.swipe(3, dx=-600)
    touchpad.pinch(4, 0.4)
    settle()
    checks.equal((columns(), active_workspace()), before, "with touchpad gestures off a 3-finger swipe leaves the row alone")
    checks.expect(not overview_open(), "and a 4-finger pinch does not open Konveyor's overview")
    touchpad.swipe(4, dy=-600)
    checks.expect(wait_for(lambda: "overview" in kde_active_effects(), 30), f"KWin's own 4-finger swipe up opens its Overview ({kde_active_effects()})")
    tap([], KEY_ESCAPE)
    checks.expect(wait_for(lambda: "overview" not in kde_active_effects(), 30), "and Escape closes it")
    load_config(default_config())
    settle()
    title = active_title()
    before = window_place(title)
    touchpad.swipe(4, dy=200)
    carried = next_place(title, before)
    checks.expect(carried[0] != before[0] and "overview" not in kde_active_effects(),
                  f"with gestures on the 4-finger swipe is Konveyor's again and carries {title} away ({before} -> {carried})")
    konveyor_action("move-window-to-workspace-up")
    settle()


def touchpad_buttons_reach_windows(checks):
    set_widths("30%")
    for button, lmr in ((touchpad.BTN_LEFT, False), (touchpad.BTN_MIDDLE, False), (touchpad.BTN_RIGHT, True), (touchpad.BTN_MIDDLE, True)):
        touchpad.add(lmr_tap_button_map=lmr)
        target = next(title for x, title in visible_columns() if title != active_title())
        x, y, width, height = frame(target)
        move(x + width / 2, y + height / 2)
        touchpad.click(button)
        checks.expect(wait_for(lambda: active_title() == target, 30), f"touchpad button {button:#x} (lmr map {lmr}) reaches {target} and focuses it")
    touchpad.remove()
    touchpad.add()
    settle()


def visible_columns():
    active = next(workspace["id"] for workspace in konveyor("Workspaces") if workspace["is_active"])
    shown = {window["title"] for window in konveyor_windows() if window["workspace_id"] == active}
    return [(x, title) for x, title in columns() if title in shown and 0 <= x < 1700]


def main():
    touchpad.add()
    move(960, 540)
    Checks().run(row_scrolls, natural_swipe_flips_the_row, workspaces_switch, pinch_toggles_overview, window_swipes,
                 other_finger_counts_pass_through, gestures_off_hand_swipes_to_kwin, touchpad_buttons_reach_windows)
    touchpad.remove()


if __name__ == "__main__":
    main()
