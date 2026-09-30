#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, config_path
from dragging import LEFT_BUTTON, META, center, dragging, grab_point, placement, quick_tiling, settled, window_json
from drophint import MAGENTA, hint_area
from fakepointer import Held, move
from kwinsession import activate, frame, konveyor, konveyor_action, open_client, wait_for

ESCAPE = 1
GREEN = (0, 255, 0)
clients = {}


def output_of(title):
    workspaces = {workspace["id"]: workspace for workspace in json.loads(konveyor("Workspaces"))}
    return workspaces[window_json(title)["workspace_id"]]["output"]


def workspace_index(title):
    workspaces = {workspace["id"]: workspace for workspace in json.loads(konveyor("Workspaces"))}
    return workspaces[window_json(title)["workspace_id"]]["idx"]


def open_rows(checks):
    move(960, 540, settle=False)
    for title in ("A", "B", "C", "D"):
        clients[title] = open_client(title)
    move(2880, 540, settle=False)
    for title in ("F", "G"):
        clients[title] = open_client(title)
    checks.equal({title: output_of(title) for title in ("A", "D", "F", "G")}, {"A": "Virtual-0", "D": "Virtual-0", "F": "Virtual-1", "G": "Virtual-1"},
                 "the rows open on their outputs")


def top_of(title):
    target = settled(title)
    return round(target[0] + target[2] / 2), round(target[1] + 20)


def escape_cancels_a_tiled_drag(checks):
    activate("B")
    before = {title: placement(title) for title in ("A", "B", "C", "D")}
    frame_before = settled("C")
    with dragging("C", top_of("B")) as pointer:
        checks.expect(wait_for(lambda: hint_area() is not None, 10, 0.5), "the drop hint shows before Escape")
        pointer.send((ESCAPE, 1), (ESCAPE, 0))
        checks.expect(wait_for(lambda: hint_area() is None, 10, 0.5), "Escape takes the drop hint away")
    checks.equal({title: placement(title) for title in ("A", "B", "C", "D")}, before, "every window is back where it was")
    checks.equal(settled("C"), frame_before, "C is back in its own spot")


def escape_cancels_a_floating_drag(checks):
    activate("C")
    checks.equal(konveyor_action("toggle-window-floating"), "", "float C")
    before = settled("C")
    with dragging("C", (700, 400)) as pointer:
        checks.expect(wait_for(lambda: settled("C") != before, 10), "the floating C follows the pointer")
        pointer.send((ESCAPE, 1), (ESCAPE, 0))
    checks.expect(wait_for(lambda: settled("C") == before, 10), f"Escape puts the floating C back ({frame('C')}, was {before})")
    checks.expect(window_json("C")["is_floating"], "C is still floating")
    checks.equal(konveyor_action("toggle-window-floating"), "", "tile C again")


def drop_into_a_stack_and_out_again(checks):
    activate("B")
    column = placement("B")[0]
    with dragging("C", top_of("B")):
        pass
    checks.equal(placement("C"), [column, 1], "C stacks on top of B")
    target = settled("C")
    with dragging("C", (round(target[0] + target[2] + 150), 540), grab=(round(target[0] + 40), round(target[1] + 40))):
        pass
    checks.equal(placement("C"), [column + 1, 1], "dragging C out of the stack gives it its own column again")
    checks.equal(placement("B"), [column, 1], "B has its column to itself")


def drop_into_tabs(checks):
    activate("B")
    checks.equal(konveyor_action("consume-or-expel-window-left"), "", "stack B under A")
    checks.equal(konveyor_action("toggle-column-tabbed-display"), "", "show A and B as tabs")
    column = placement("B")[0]
    with dragging("C", top_of("B")):
        pass
    checks.equal(placement("C")[0], column, f"C joins the tabbed column ({placement('C')})")
    checks.expect(wait_for(lambda: settled("C")[:2] == settled("B")[:2]), f"C shows as a tab in B's place ({frame('C')}, {frame('B')})")
    for title in ("B", "C"):
        activate(title)
        checks.equal(konveyor_action("consume-or-expel-window-right"), "", f"take {title} back out of the tabs")
    checks.expect(wait_for(lambda: len({placement(title)[0] for title in ("A", "B", "C")}) == 3), "A, B and C have their own columns again")


def switch_workspace_mid_drag(checks):
    activate("D")
    home = workspace_index("D")
    with dragging("D", (960, 540)):
        checks.equal(konveyor_action("focus-workspace-down"), "", "switch to the workspace below mid-drag")
        checks.expect(wait_for(lambda: not 0 <= settled("A")[1] < 1080, 10), f"the row moves away with the workspace switch ({frame('A')})")
    checks.equal(workspace_index("D"), home + 1, "D drops onto the workspace it was carried to")
    checks.equal(workspace_index("A"), home, "A stays behind")
    checks.equal(konveyor_action("move-window-to-workspace-up"), "", "send D back")
    checks.equal(konveyor_action("focus-workspace-up"), "", "go back up")


def drop_on_other_output(checks):
    activate("C")
    activate("F")
    target = settled("F")
    goal = (round(target[0] + target[2] + 150), 540)
    with dragging("C", goal):
        after_f = lambda area: area and 1920 <= area[0] < target[0] + target[2] < area[2]
        checks.expect(wait_for(lambda: after_f(hint_area(GREEN)), 10, 0.5), f"Virtual-1 draws its own green hint after F ({hint_area(GREEN)}, F at {target})")
        area = hint_area(MAGENTA)
        checks.expect(area is None, f"Virtual-0's magenta hint is not drawn ({area})")
    checks.equal(output_of("C"), "Virtual-1", "C dropped on Virtual-1")
    checks.equal(placement("C"), [placement("F")[0] + 1, 1], "right after F")
    with dragging("C", (960, 540), grab=grab_point("C", 1920)):
        pass
    checks.equal(output_of("C"), "Virtual-0", "C can be dragged back to Virtual-0")


def close_mid_drag(checks):
    activate("D")
    start = center("D")
    with dragging("D", (start[0] - 400, 540)):
        checks.expect(wait_for(lambda: hint_area() is not None, 10, 0.5), "the hint shows while D is dragged")
        clients.pop("D").kill()
        checks.expect(wait_for(lambda: "D" not in [window["title"] for window in json.loads(konveyor("Windows"))], 10), "D goes away mid-drag")
        checks.expect(wait_for(lambda: hint_area() is None, 10, 0.5), "its drop hint goes with it")
        checks.equal(quick_tiling(), "true|true", "KWin's quick tiling comes back once the dragged window is gone")
    activate("B")
    column = placement("B")[0]
    with dragging("C", top_of("B")):
        pass
    checks.equal(placement("C"), [column, 1], "dragging still works afterwards")


def use_titlebar_drag(mode):
    text = config_path().read_text()
    config_path().write_text(text.replace('titlebar-drag "move-window"', f'titlebar-drag "{mode}"').replace('titlebar-drag "scroll-view"', f'titlebar-drag "{mode}"'))
    return konveyor("LoadConfigFile", "")


def scroll_view_drag(checks):
    checks.equal(use_titlebar_drag("scroll-view"), "", "make dragging a tiled window scroll the row")
    activate("B")
    before = {title: placement(title) for title in ("A", "B", "C")}
    start = settled("B")
    with Held() as pointer:
        grab = grab_point("B")
        pointer.send((META, 1), f"move:{grab[0]}:{grab[1]}", f"button:{LEFT_BUTTON}:1")
        pointer.glide(grab, (grab[0] - 500, grab[1]), 12)
        checks.expect(wait_for(lambda: frame("B")[0] < start[0] - 300, 10), f"the row follows the pointer ({frame('B')}, was {start})")
        checks.expect(hint_area() is None, "scrolling the row shows no drop hint")
        pointer.send(f"button:{LEFT_BUTTON}:0", (META, 0))
    checks.equal({title: placement(title) for title in ("A", "B", "C")}, before, "no window changed its place in the row")
    checks.equal(use_titlebar_drag("move-window"), "", "make dragging move windows again")


def main():
    Checks().run(open_rows, escape_cancels_a_tiled_drag, escape_cancels_a_floating_drag, drop_into_a_stack_and_out_again, drop_into_tabs,
                 switch_workspace_mid_drag, drop_on_other_output, scroll_view_drag, close_mid_drag)


if __name__ == "__main__":
    main()
