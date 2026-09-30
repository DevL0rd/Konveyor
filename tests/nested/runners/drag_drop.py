#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from dragging import dragging, placement, settled, window_json
from drophint import hint_area
from fakepointer import move
from kwinsession import activate, frame, konveyor, konveyor_action, open_client, wait_for

ESCAPE = 1
clients = {}


def output_of(title):
    workspaces = {workspace["id"]: workspace for workspace in json.loads(konveyor("Workspaces"))}
    return workspaces[window_json(title)["workspace_id"]]["output"]


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


def main():
    Checks().run(open_rows, escape_cancels_a_tiled_drag, escape_cancels_a_floating_drag)


if __name__ == "__main__":
    main()
