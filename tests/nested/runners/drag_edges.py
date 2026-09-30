#!/usr/bin/env python3
import json
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from fakepointer import Held, move
from kwinsession import activate, for_window, konveyor, konveyor_action, konveyor_windows, managed_titles, open_client, run_script, wait_for
from nested import build_dir

META = 125
LEFT_BUTTON = 0x110


def outputs_by_workspace():
    return {workspace["id"]: workspace["output"] for workspace in json.loads(konveyor("Workspaces"))}


def placed(title):
    window = next(window for window in konveyor_windows() if window["title"] == title)
    return outputs_by_workspace()[window["workspace_id"]], window["layout"]["tile_pos_in_workspace_view"][0]


def frame(title):
    printed = run_script(for_window(title, 'const g = w.frameGeometry; print("MARK|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height);'))
    return tuple(float(value) for value in printed[0].split("|"))


def settled(title):
    window = next(window for window in konveyor_windows() if window["title"] == title)
    target = (*window["layout"]["tile_pos_in_workspace_view"], *window["layout"]["tile_size"])
    return all(abs(a - b) < 1 for a, b in zip(frame(title), target))


def center(title):
    x, y, width, height = frame(title)
    return round(x + width / 2), round(y + height / 2)


def log_has(text):
    with open(os.environ["KONVEYOR_KWIN_LOG"], errors="replace") as log:
        return any(text in line for line in log)


def open_rows(checks):
    move(960, 540, settle=False)
    for title in ("A", "B", "C", "D"):
        open_client(title)
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    subprocess.Popen([str(build_dir() / "bin" / "drag_source_client"), "S"], stdout=log, stderr=subprocess.STDOUT)
    checks.expect(wait_for(lambda: "S" in managed_titles(), 60), "the drag source opened")
    move(2880, 540, settle=False)
    for title in ("F", "G", "H"):
        open_client(title)
    checks.equal({title: placed(title)[0] for title in ("A", "S", "F", "H")}, {"A": "Virtual-0", "S": "Virtual-0", "F": "Virtual-1", "H": "Virtual-1"},
                 "the rows open on their outputs")


def visible_on_first_output():
    return next(title for title in ("D", "C", "B", "A") if 100 < center(title)[0] < 1820)


def window_drag_held(checks):
    title = visible_on_first_output()
    start = center(title)
    with Held() as pointer:
        pointer.send((META, 1), f"move:{start[0]}:{start[1]}", f"button:{LEFT_BUTTON}:1")
        pointer.glide(start, (4, 540))
        before = placed("A")[1]
        checks.expect(wait_for(lambda: placed("A")[1] > before + 150, 30), f"holding a dragged window still at the left edge keeps scrolling ({before} -> {placed('A')[1]})")
        pointer.send(f"button:{LEFT_BUTTON}:0", (META, 0))
    checks.expect(wait_for(lambda: placed(title)[0] == "Virtual-0" and 0 <= frame(title)[0] < 1920 and 0 <= frame(title)[1] < 1080),
                  f"the dropped window is on screen ({placed(title)}, {frame(title)})")


def file_drag_on_other_output(checks):
    activate("S")
    checks.expect(wait_for(lambda: settled("S")), "the drag source is in place")
    start = center("S")
    with Held() as pointer:
        pointer.send(f"move:{start[0]}:{start[1]}", f"button:{LEFT_BUTTON}:1")
        pointer.glide(start, (start[0] + 80, start[1]), 8)
        checks.expect(wait_for(lambda: log_has("konveyor-test-drag-started")), "the client started a drag and drop")
        pointer.glide((start[0] + 80, start[1]), (3835, 540))
        first, other = placed("F")[1], placed("A")[1]
        checks.expect(wait_for(lambda: placed("F")[1] < first - 150, 30), f"a file held at the right edge of Virtual-1 scrolls Virtual-1 ({first} -> {placed('F')[1]})")
        checks.equal(placed("A")[1], other, "Virtual-0, where the drag started, does not scroll")
        pointer.send(f"button:{LEFT_BUTTON}:0")


def drag_overview_unplug(checks):
    title = "H"
    start = center(title)
    with Held() as pointer:
        pointer.send((META, 1), f"move:{start[0]}:{start[1]}", f"button:{LEFT_BUTTON}:1")
        pointer.glide(start, (3835, 540))
        checks.equal(konveyor_action("toggle-overview"), "", "open the overview mid-drag")
        result = subprocess.run(["kscreen-doctor", "output.Virtual-1.disable"], capture_output=True, text=True)
        checks.expect(result.returncode == 0, f"unplug Virtual-1 mid-drag ({result.stderr.strip()})")
        checks.expect(wait_for(lambda: [output["name"] for output in json.loads(konveyor("Outputs"))] == ["Virtual-0"]), "Konveyor sees one output")
        pointer.send(f"button:{LEFT_BUTTON}:0", (META, 0))
    konveyor_action("close-overview")
    checks.expect(wait_for(lambda: not json.loads(konveyor("OverviewState"))["is_open"]), "the overview closes")
    checks.expect(wait_for(lambda: all(placed(name)[0] == "Virtual-0" for name in ("A", "F", "G", "H"))), "every window moved to Virtual-0")
    activate(title)
    checks.expect(wait_for(lambda: settled(title) and 0 <= frame(title)[0] < 1920 and 0 <= frame(title)[1] < 1080),
                  f"the dragged {title} can be brought on screen ({frame(title)})")


def main():
    Checks().run(open_rows, window_drag_held, file_drag_on_other_output, drag_overview_unplug)


if __name__ == "__main__":
    main()
