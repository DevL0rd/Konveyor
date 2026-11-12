#!/usr/bin/env python3
import sys
import threading
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import konveyor_action, open_client, run_script, wait_for

WATCH = """
function state(kind) {
    print("MARK|" + kind + "|" + workspace.desktops.length + "|" + workspace.desktopGridWidth + "x" + workspace.desktopGridHeight);
}
workspace.currentDesktopChanged.connect(() => state("current"));
"""


def grid_order(checks):
    checks.equal(konveyor_action("focus-workspace", "1"), "", "start on the first workspace")
    open_client("Grid A")
    checks.expect(wait_for(lambda: int(run_script('print("MARK|" + workspace.desktops.length);')[0]) == 2), "one occupied workspace and the empty one")
    seen = []
    watcher = threading.Thread(target=lambda: seen.extend(run_script(WATCH, seconds=4)))
    watcher.start()
    time.sleep(1.5)
    checks.equal(konveyor_action("move-window-to-workspace-down"), "", "move the only window into the new empty workspace")
    watcher.join()
    switches = [line.split("|")[1:] for line in seen if line.startswith("current|")]
    checks.expect(len(switches) >= 1, f"KDE switched desktops ({seen})")
    checks.expect(all(grid == f"1x{count}" for count, grid in switches), f"every desktop switch sees the one column layout of all desktops ({switches})")


def main():
    Checks().run(grid_order)


if __name__ == "__main__":
    main()
