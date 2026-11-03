#!/usr/bin/env python3
import os
import signal
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from dragging import dragging, quick_tiling, settled
from kwinsession import (
    for_window,
    managed_titles,
    reload_konveyor,
    run_script,
    wait_for,
)


def close_window(title):
    run_script(for_window(title, "w.closeWindow();"))


def goal_right_of(title):
    x, y, width, height = settled(title)
    return round(x + width + 150), 540


def window_closed_during_drag(checks):
    with dragging("C", goal_right_of("B")) as pointer:
        checks.expect(
            quick_tiling().startswith("false"),
            f"quick tiling is held while C is dragged ({quick_tiling()})",
        )
        close_window("C")
        checks.expect(
            wait_for(lambda: "C" not in managed_titles()), "C closed mid-drag"
        )
        checks.equal(
            quick_tiling(),
            "true|true",
            "quick tiling comes back when the dragged window closes",
        )
        pointer.glide(goal_right_of("B"), (900, 700), 6)
    checks.equal(managed_titles(), ["A", "B"], "the other windows are untouched")


def effect_reloaded_during_drag(checks):
    with dragging("B", goal_right_of("A")) as pointer:
        checks.expect(
            reload_konveyor(["A", "B"]), "the effect reloads while B is dragged"
        )
        pointer.glide(goal_right_of("A"), (900, 700), 6)
    checks.expect(
        wait_for(lambda: settled("B") and quick_tiling() == "true|true"),
        f"B settles and quick tiling is back ({quick_tiling()})",
    )
    checks.equal(managed_titles(), ["A", "B"], "both windows are still managed")


def client_killed_during_drag(checks):
    pid = int(run_script(for_window("B", 'print("MARK|" + w.pid);'))[0])
    with dragging("B", goal_right_of("A")) as pointer:
        os.kill(pid, signal.SIGKILL)
        checks.expect(
            wait_for(lambda: "B" not in managed_titles()), "B vanished mid-drag"
        )
        checks.expect(
            wait_for(lambda: quick_tiling() == "true|true"),
            f"quick tiling comes back when the dragged client dies ({quick_tiling()})",
        )
        pointer.glide(goal_right_of("A"), (900, 700), 6)
    checks.equal(managed_titles(), ["A"], "the other window is untouched")


def main():
    Checks().run(
        window_closed_during_drag,
        effect_reloaded_during_drag,
        client_killed_during_drag,
    )


if __name__ == "__main__":
    main()
