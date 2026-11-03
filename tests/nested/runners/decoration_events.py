#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from borderprobe import CYAN, MAGENTA, TITLES, expect_painted, painted_titles, probe_config, screenshot, settle_to, solid
from checks import Checks, load_config
from kwinsession import active_title, for_window, frame, konveyor, konveyor_action, run_script, wait_for

STYLES = solid("Mark", "#ff00ff") + solid("Tint", "#00ffff") + 'border-style "Empty" {\n}\n'


def load(events):
    return load_config(probe_config(events, STYLES))




def gap(first, second):
    ax, ay, aw, ah = frame(first)
    bx, by, bw, bh = frame(second)
    return max(ax - (bx + bw), bx - (ax + aw), ay - (by + bh), by - (ay + ah), 0)


def row_is_visible(checks):
    xs = [frame(title)[0] for title in TITLES]
    widths = [frame(title)[2] for title in TITLES]
    checks.expect(all(x >= 0 and x + width <= 1920 for x, width in zip(xs, widths)), f"A, B and C all sit on the output ({xs}, {widths})")


def reach_follows_the_effect(checks):
    checks.expect(load('        on "focus" "Mark" reach="self" duration=30'), "a focus effect with reach self loads")
    settle_to("B")
    settle_to("A")
    expect_painted(checks, MAGENTA, {"A"}, "reach self paints only the focused window")
    checks.expect(load('        on "focus" "Mark" reach="neighbors" duration=30'), "a focus effect with reach neighbors loads")
    settle_to("B")
    settle_to("A")
    expect_painted(checks, MAGENTA, {"A", "B"}, "reach neighbors paints the focused window and the column beside it, not the far one")
    settle_to("B")
    expect_painted(checks, MAGENTA, {"A", "B", "C"}, "neighbors of the middle column are both sides")
    checks.expect(load('        on "focus" "Mark" reach="all" duration=30'), "a focus effect with reach all loads")
    settle_to("A")
    settle_to("C")
    expect_painted(checks, MAGENTA, {"A", "B", "C"}, "reach all paints every window on the workspace")


def a_new_focus_replaces_the_running_one(checks):
    checks.expect(load('        on "focus" "Mark" reach="self" duration=30'), "reach self loads again")
    settle_to("A")
    settle_to("B")
    expect_painted(checks, MAGENTA, {"B"}, "moving focus replaces the focus effect on the old window")
    settle_to("B")
    expect_painted(checks, MAGENTA, {"B"}, "asking for the focused window again changes nothing")


def unfocus_paints_the_window_that_lost_focus(checks):
    checks.expect(load('        on "unfocus" "Tint" reach="self" duration=30'), "an unfocus effect loads")
    settle_to("A")
    settle_to("B")
    expect_painted(checks, CYAN, {"A"}, "the window that lost focus gets the unfocus effect")
    expect_painted(checks, MAGENTA, set(), "and the focused window gets none")


def losing_focus_altogether_plays_unfocus_once(checks):
    settle_to("A")
    run_script("workspace.activeWindow = null;")
    checks.expect(wait_for(lambda: active_title() == "none"), "no window has focus")
    expect_painted(checks, CYAN, {"A"}, "dropping focus plays the unfocus effect on the window that had it")
    settle_to("B")
    expect_painted(checks, CYAN, set(), "focusing another window afterwards replaces it and does not play the unfocus effect twice")
    run_script("workspace.activeWindow = null;")
    expect_painted(checks, CYAN, {"B"}, "the next loss plays on the window that held focus last")
    run_script("workspace.activeWindow = null;")
    settle_to("C")


def nothing_plays_without_a_style(checks):
    for name, events in (("none", 'on "focus" "none" reach="all" duration=30'), ("unknown", 'on "focus" "Ghost" reach="all" duration=30'),
                         ("empty", 'on "focus" "Empty" reach="all" duration=30')):
        load(f"        {events}")
        settle_to("A")
        settle_to("B")
        checks.expect(not painted_titles(screenshot(), TITLES, MAGENTA) and not painted_titles(screenshot(), TITLES, CYAN), f"a {name} style paints nothing")


def urgent_windows_play_the_urgent_effect(checks):
    checks.expect(load('        on "urgent" "Mark" reach="self" duration=3'), "an urgent effect loads")
    settle_to("A")
    run_script(for_window("C", "w.demandsAttention = true;"))
    expect_painted(checks, MAGENTA, {"C"}, "a window asking for attention plays the urgent effect")
    run_script(for_window("C", "w.demandsAttention = false;"))
    run_script(for_window("B", "w.demandsAttention = true;"))
    expect_painted(checks, MAGENTA, {"B", "C"}, "another urgent window plays its own and the first keeps playing")
    run_script(for_window("B", "w.demandsAttention = false;"))
    expect_painted(checks, MAGENTA, set(), "urgent effects end on their own", 30)


def windows_that_disappear_leave_the_play(checks):
    checks.expect(load('        on "focus" "Mark" reach="all" duration=30'), "reach all loads for the removal check")
    settle_to("A")
    settle_to("C")
    expect_painted(checks, MAGENTA, {"A", "B", "C"}, "everything is painted")
    run_script(for_window("B", "w.minimized = true;"))
    expect_painted(checks, MAGENTA, {"A", "C"}, "a window minimised during the play is left out")
    run_script(for_window("B", "w.minimized = false;"))
    run_script(for_window("C", "w.closeWindow();"))
    checks.expect(wait_for(lambda: frame("C") is None, 20), "C closed")
    expect_painted(checks, MAGENTA, {"A", "B"}, "closing a window moves focus on and the effect plays on the windows left")
    settle_to("A")
    expect_painted(checks, MAGENTA, {"A", "B"}, "a later focus change still plays on the remaining windows")


def floating_windows_count_as_neighbours_by_distance(checks):
    checks.expect(load('        on "focus" "Mark" reach="neighbors" duration=30'), "reach neighbors loads for floating windows")
    settle_to("B")
    checks.equal(konveyor_action("toggle-window-floating"), "", "float B")
    settle_to("A")
    settle_to("B")
    near = {title for title in ("A", "C") if gap("B", title) <= 32}
    checks.expect(near, f"the floating B still sits within a window gap of a tiled one ({near})")
    expect_painted(checks, MAGENTA, {"B"} | near, "a floating window reaches the tiled windows it sits next to")
    move = {"name": "move-floating-window", "arguments": [], "properties": {"x": "1750", "y": "300"}}
    checks.equal(konveyor("Action", json.dumps(move)), "", "move B away from the row")
    checks.expect(wait_for(lambda: frame("B")[0] > 1700, 20), f"B moved away from the row ({frame('B')})")
    settle_to("A")
    settle_to("B")
    expect_painted(checks, MAGENTA, {"B"}, "a floating window far from the tiled ones has no neighbours, even in the next column")
    checks.equal(konveyor_action("toggle-window-floating"), "", "tile B again")


def minimised_windows_are_not_reached(checks):
    checks.expect(load('        on "focus" "Mark" reach="all" duration=30'), "reach all loads for the minimised check")
    run_script(for_window("C", "w.minimized = true;"))
    settle_to("A")
    settle_to("B")
    expect_painted(checks, MAGENTA, {"A", "B"}, "a window that is minimised when the effect starts is left out")
    run_script(for_window("C", "w.minimized = false;"))


def every_event_can_be_bound(checks):
    events = "\n".join(f'        on "{name}" "Mark" reach="all" duration=30'
                       for name in ("focus", "unfocus", "click", "scroll", "keypress", "urgent", "beat", "view"))
    checks.expect(load(events), "an effect on every event loads")
    settle_to("A")
    settle_to("B")
    expect_painted(checks, MAGENTA, set(TITLES), "focus still plays with every event bound")
    checks.expect(json.loads(konveyor("SignalFrames"))["observing"], "and the input signals are observed for the click, scroll and key events")


def wide_gaps_leave_neighbours_to_the_layout(checks):
    events = '        on "focus" "Mark" reach="neighbors" duration=30'
    checks.expect(load_config(probe_config(events, STYLES, gaps=48)), "reach neighbors with wide gaps loads")
    checks.expect(wait_for(lambda: gap("A", "B") >= 48 and gap("B", "C") >= 48, 20), f"the columns sit a wide gap apart ({gap('A', 'B')})")
    settle_to("B")
    settle_to("A")
    expect_painted(checks, MAGENTA, {"A", "B"}, "the next column is a neighbour although it is further than a window gap away")
    settle_to("B")
    checks.equal(konveyor_action("consume-or-expel-window-left"), "", "stack B under A")
    settle_to("C")
    checks.equal(konveyor_action("consume-or-expel-window-left"), "", "stack C under them")
    checks.expect(wait_for(lambda: frame("A")[0] == frame("B")[0] == frame("C")[0], 20), "all three share one column")
    settle_to("B")
    settle_to("A")
    expect_painted(checks, MAGENTA, {"A", "B"}, "in a stacked column only the window right below is a neighbour")
    for title in ("C", "B"):
        settle_to(title)
        checks.equal(konveyor_action("expel-window-from-column"), "", f"{title} gets its own column again")
    checks.expect(wait_for(lambda: len({frame(title)[0] for title in TITLES}) == 3, 20), "the three columns are back")


def main():
    Checks().run(row_is_visible, reach_follows_the_effect, a_new_focus_replaces_the_running_one, unfocus_paints_the_window_that_lost_focus,
                 losing_focus_altogether_plays_unfocus_once, nothing_plays_without_a_style, urgent_windows_play_the_urgent_effect,
                 minimised_windows_are_not_reached, every_event_can_be_bound, floating_windows_count_as_neighbours_by_distance,
                 wide_gaps_leave_neighbours_to_the_layout, windows_that_disappear_leave_the_play)


if __name__ == "__main__":
    main()
