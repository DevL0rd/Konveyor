#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from fakepointer import move
from kwinsession import activate, frames, intersects, konveyor, konveyor_windows, open_client, wait_for

ROWS = {"Virtual-0": ("A1", "A2", "A3", "A4"), "Virtual-1": ("B1", "B2", "B3", "B4"), "Virtual-2": ("C1", "C2", "C3", "C4")}


def outputs():
    return {entry["name"]: (entry["logical"]["x"], entry["logical"]["y"], entry["logical"]["width"], entry["logical"]["height"])
            for entry in json.loads(konveyor("Outputs"))}


def home_of(title):
    return next(output for output, titles in ROWS.items() if title in titles)


def misplaced():
    screens = outputs()
    wrong = []
    for title, frame in frames().items():
        home = screens[home_of(title)]
        if intersects(frame, home):
            continue
        others = [name for name, rect in screens.items() if name != home_of(title) and intersects(frame, rect)]
        if others:
            wrong.append((title, frame, others))
    return wrong


def settled():
    targets = {window["title"]: window["layout"]["tile_size"] for window in konveyor_windows()}
    return all(abs(frame[2] - targets[title][0]) < 1 for title, frame in frames().items() if title in targets)


def l_shape(checks):
    result = subprocess.run(["kscreen-doctor", "output.Virtual-2.position.0,1080"], capture_output=True, text=True)
    checks.expect(result.returncode == 0, f"move Virtual-2 below Virtual-0 ({result.stderr.strip()})")
    expected = {"Virtual-0": (0, 0), "Virtual-1": (1920, 0), "Virtual-2": (0, 1080)}
    checks.expect(wait_for(lambda: {name: rect[:2] for name, rect in outputs().items()} == expected), f"the outputs form an L ({outputs()})")


def open_rows(checks):
    for output, titles in ROWS.items():
        x, y, width, height = outputs()[output]
        move(x + width / 2, y + height / 2, settle=False)
        for title in titles:
            open_client(title)
    placed = {window["title"]: window for window in konveyor_windows()}
    checks.equal(sorted(placed), sorted(title for titles in ROWS.values() for title in titles), "every window is managed")


def scroll_rows(checks):
    for titles in ROWS.values():
        for title in (*titles, *reversed(titles)):
            activate(title)
            checks.expect(wait_for(settled), f"the layout settles with {title} focused")
            wrong = misplaced()
            checks.expect(not wrong, f"with {title} focused no window shows up on another output ({wrong})")


def main():
    Checks().run(l_shape, open_rows, scroll_rows)


if __name__ == "__main__":
    main()
