#!/usr/bin/env python3
import json
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from fakepointer import move
from kwinsession import activate, frames, intersects, konveyor, konveyor_windows, managed_titles, open_client, run_script, wait_for
from nested import build_dir

EVENTS = Path(os.environ["KONVEYOR_TEST_ROOT"]) / "scale-events.txt"
HOME = "Virtual-0"
OTHER = "Virtual-1"
TITLE = "A"
WATCH_SECONDS = 4
WATCH_WINDOW = f'const w = workspace.windowList().find(w => !w.deleted && w.caption == "{TITLE}");'
TRAIL = 'const show = () => print("MARK|" + [w.frameGeometry.x, w.frameGeometry.y, w.frameGeometry.width, w.frameGeometry.height].join("|")); show(); w.frameGeometryChanged.connect(show);'
tracker = []


def outputs():
    return {entry["name"]: (entry["logical"]["x"], entry["logical"]["y"], entry["logical"]["width"], entry["logical"]["height"])
            for entry in json.loads(konveyor("Outputs"))}


def events(kind):
    return [line for line in EVENTS.read_text().splitlines() if line.startswith(kind + " ")]


def on_screen(frame):
    return any(intersects(frame, rect) for rect in outputs().values())


def inside(frame, rect):
    return frame[0] >= rect[0] and frame[1] >= rect[1] and frame[0] + frame[2] <= rect[0] + rect[2] and frame[1] + frame[3] <= rect[1] + rect[3]


def settled():
    targets = {window["title"]: window["layout"]["tile_size"] for window in konveyor_windows()}
    return all(abs(frame[2] - targets[title][0]) < 1 for title, frame in frames().items() if title in targets)


def trail(extra="", seconds=WATCH_SECONDS):
    return [tuple(float(value) for value in line.split("|")) for line in run_script(WATCH_WINDOW + TRAIL + extra, seconds)]


def parked():
    return not on_screen(frames()[TITLE])


def kscreen(*settings):
    return subprocess.run(["kscreen-doctor", *settings], capture_output=True, text=True).returncode == 0


def mixed_scale_layout(checks):
    checks.expect(kscreen(f"output.{HOME}.scale.1.5", f"output.{OTHER}.scale.2"), "the outputs take different scales")
    checks.expect(wait_for(lambda: outputs()[HOME][2] == 1280 and outputs()[OTHER][2] == 960), f"the scales apply ({outputs()})")
    checks.expect(kscreen(f"output.{HOME}.position.960,0", f"output.{OTHER}.position.0,360"), "the scale 2 output moves left of the scale 1.5 one")
    checks.expect(wait_for(lambda: outputs()[HOME][:2] == (960, 0) and outputs()[OTHER][:2] == (0, 360)), f"the outputs overlap in rows ({outputs()})")


def open_row(checks):
    x, y, width, height = outputs()[HOME]
    move(x + width / 2, y + height / 2, settle=False)
    with EVENTS.open("w") as log:
        tracker.append(subprocess.Popen([str(build_dir() / "bin" / "scale_tracking_client"), TITLE], stdout=log, stderr=subprocess.STDOUT))
    checks.expect(wait_for(lambda: TITLE in managed_titles() and events("MAPPED"), 30), f"{TITLE} is managed and mapped")
    for title in ("B", "C", "D"):
        open_client(title)
    checks.expect(wait_for(settled), "the layout settles with D focused")


def parked_window_never_loops(checks):
    checks.expect(wait_for(parked), f"{TITLE} is parked off every output ({frames()[TITLE]}, {outputs()})")
    time.sleep(1)
    scales_before = len(events("SCALE"))
    configures_before = len(events("CONFIGURE"))
    frames_seen = trail()
    scale_changes = len(events("SCALE")) - scales_before
    configures = len(events("CONFIGURE")) - configures_before
    summary = f"{scale_changes} scale changes, {configures} configures, {len(frames_seen)} frames in {WATCH_SECONDS}s"
    checks.expect(scale_changes == 0, f"the parked window's preferred scale holds still ({summary})")
    checks.expect(configures == 0, f"the parked window gets no configures ({summary})")
    checks.equal([frame for frame in frames_seen if on_screen(frame)], [], f"{TITLE} stays off every output ({summary})")


def parked_window_ignores_a_configure(checks):
    configures_before = len(events("CONFIGURE"))
    resize = "const g = w.frameGeometry; w.frameGeometry = {x: g.x, y: g.y, width: g.width + 40, height: g.height};"
    frames_seen = trail(resize, 2)
    checks.expect(len(events("CONFIGURE")) > configures_before, "the parked window answers a configure")
    checks.equal([frame for frame in frames_seen if on_screen(frame)], [], f"answering a configure keeps {TITLE} off every output")


def unparked_window_returns_and_confines(checks):
    checks.expect(activate(TITLE), f"{TITLE} is focused")
    checks.expect(wait_for(settled), f"the layout settles with {TITLE} focused")
    checks.expect(wait_for(lambda: inside(frames()[TITLE], outputs()[HOME])), f"{TITLE} is back on {HOME} ({frames()[TITLE]}, {outputs()[HOME]})")
    throw = "const g = w.frameGeometry; w.frameGeometry = {x: -5000, y: -5000, width: g.width + 40, height: g.height};"
    configures_before = len(events("CONFIGURE"))
    frames_seen = trail(throw, 2)
    checks.expect(len(events("CONFIGURE")) > configures_before, "the unparked window answers a configure")
    checks.equal([frame for frame in frames_seen if not on_screen(frame)], [], f"KWin confines {TITLE} again once it is back in view")


def main():
    try:
        Checks().run(mixed_scale_layout, open_row, parked_window_never_loops, parked_window_ignores_a_configure, unparked_window_returns_and_confines)
    finally:
        for process in tracker:
            process.terminate()


if __name__ == "__main__":
    main()
