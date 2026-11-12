#!/usr/bin/env python3
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, config_path, load_config
from kwinsession import frame, konveyor_action, wait_for
from screenshot import capture_workspace

GAPS = 16
WIDTH = 1920
SHOTS = Path(sys.argv[1]) if len(sys.argv) > 1 and sys.argv[1] else Path(os.environ["KONVEYOR_REPORT"]).parent


def centered(title):
    geometry = frame(title)
    return geometry is not None and abs(geometry[0] + geometry[2] / 2 - WIDTH / 2) <= 2


def right_after(left, right):
    a, b = frame(left), frame(right)
    return a is not None and b is not None and abs(a[0] + a[2] + GAPS - b[0]) <= 2


def settle_on(action, title):
    konveyor_action(action)
    return wait_for(lambda: centered(title), 30, 0.2)


def shot(name):
    SHOTS.mkdir(parents=True, exist_ok=True)
    capture_workspace(str(SHOTS / f"{name}.png"))


def set_wrap(on):
    text = config_path().read_text()
    return load_config(text.replace(f"wrap-columns {'false' if on else 'true'}", f"wrap-columns {'true' if on else 'false'}"))


def wrapped(checks):
    checks.expect(set_wrap(True), "wrap-columns true loads")
    checks.expect(settle_on("focus-column-last", "D"), f"the last column D is centered ({frame('D')})")
    checks.expect(wait_for(lambda: right_after("D", "A"), 30, 0.2), f"the first column A follows D on the right ({frame('D')}, {frame('A')})")
    checks.expect(right_after("C", "D"), f"C still sits left of D ({frame('C')}, {frame('D')})")
    shot("on-last")
    checks.expect(settle_on("focus-column-right", "A"), f"focus-column-right from D centers A ({frame('A')})")
    checks.expect(wait_for(lambda: right_after("D", "A") and right_after("A", "B"), 30, 0.2),
                  f"D sits left of A and B right of it ({frame('D')}, {frame('A')}, {frame('B')})")
    shot("on-first")
    checks.expect(settle_on("focus-column-left", "D"), f"focus-column-left from A goes back around to D ({frame('D')})")
    checks.expect(settle_on("move-column-to-first", "D"), f"move-column-to-first keeps D centered ({frame('D')})")
    checks.expect(wait_for(lambda: right_after("D", "A") and right_after("A", "B"), 30, 0.2),
                  f"after the move D is followed by A and B ({frame('D')}, {frame('A')}, {frame('B')})")
    checks.expect(wait_for(lambda: right_after("C", "D"), 30, 0.2), f"C wraps to the left of D across the seam ({frame('C')}, {frame('D')})")
    shot("on-moved")
    checks.expect(settle_on("move-column-to-last", "D"), f"move-column-to-last puts D back at the end ({frame('D')})")


def plain(checks):
    checks.expect(set_wrap(False), "wrap-columns false loads")
    checks.expect(settle_on("focus-column-last", "D"), f"the last column D is centered ({frame('D')})")
    checks.expect(wait_for(lambda: frame("A")[0] + frame("A")[2] <= 0, 30, 0.2), f"A stays at the far left of the row ({frame('A')})")
    shot("off-last")
    konveyor_action("focus-column-right")
    checks.expect(wait_for(lambda: centered("D"), 10, 0.2), "focus-column-right at the last column stays on D")
    checks.expect(settle_on("focus-column-first", "A"), f"the first column A is centered ({frame('A')})")
    checks.expect(frame("D")[0] >= WIDTH, f"D stays at the far right of the row ({frame('D')})")
    shot("off-first")


if __name__ == "__main__":
    Checks().run(wrapped, plain)
