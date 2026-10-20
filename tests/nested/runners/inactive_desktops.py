#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import frame, intersects, konveyor, konveyor_action, open_client, run_script, wait_for


def screen():
    x, y, width, height = (float(value) for value in run_script('const g = workspace.activeScreen.geometry; print("MARK|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height);')[0].split("|"))
    return (x, y, width, height)


def on_screen(title):
    placed = frame(title)
    return placed is not None and intersects(placed, screen())


def overview_open():
    return json.loads(konveyor("OverviewState"))["is_open"]


def inactive_windows(checks):
    checks.equal(konveyor_action("focus-workspace", "1"), "", "start on the first workspace")
    open_client("Inactive A")
    konveyor_action("focus-workspace", "2")
    open_client("Inactive B")
    konveyor_action("focus-workspace", "1")
    checks.expect(wait_for(lambda: on_screen("Inactive B")), "a window on an inactive desktop stays on screen")
    home = frame("Inactive B")
    checks.equal(konveyor_action("open-overview"), "", "open the overview")
    checks.expect(wait_for(overview_open), "the overview is open")
    checks.expect(on_screen("Inactive B"), "the overview leaves the inactive desktop's window on screen")
    checks.equal(frame("Inactive B"), home, "the inactive desktop's window keeps its local frame in the overview")
    konveyor_action("close-overview")
    checks.expect(wait_for(lambda: not overview_open()), "the overview closes")
    checks.equal(frame("Inactive B"), home, "the local frame is unchanged after the overview")


def main():
    Checks().run(inactive_windows)


if __name__ == "__main__":
    main()
