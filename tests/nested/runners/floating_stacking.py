#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from dragging import settled
from fakepointer import fake, move
from kwinsession import activate, active_title, frame, konveyor_action, run_script, wait_for
from nested import build_dir

WATCHED = ("A", "B", "C")


def order():
    titles = ", ".join(f'"{title}"' for title in WATCHED)
    return run_script(f'for (const w of workspace.stackingOrder) {{ if (!w.deleted && [{titles}].includes(w.caption)) print("MARK|" + w.caption); }}')


def above(upper, lower):
    stack = order()
    return upper in stack and lower in stack and stack.index(upper) > stack.index(lower)


def float_window(title):
    activate(title)
    konveyor_action("toggle-window-floating")
    return settled(title)


def clicking_the_active_tiled_window_leaves_floating_on_top(checks):
    float_window("C")
    activate("A")
    x, y, _, _ = frame("A")
    move(round(x) + 40, round(y) + 40)
    fake("click", round(x) + 40, round(y) + 40, settle=0.5)
    checks.equal(active_title(), "A", "A is active after the click")
    checks.expect(wait_for(lambda: above("C", "A") and above("C", "B"), 5), f"the floating C stays above the tiled windows after a click on A ({order()})")
    activate("C")
    konveyor_action("move-window-to-tiling")
    settled("C")


def activating_on_one_output_leaves_the_other_alone(checks):
    activate("C")
    konveyor_action("move-column-to-monitor-right")
    activate("A")
    activate("C")
    checks.expect(wait_for(lambda: above("C", "A") and above("C", "B"), 5), f"activating C on the second output keeps it above the first output's windows ({order()})")


def open_overlay(title, output):
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    process = subprocess.Popen([str(build_dir() / "bin" / "tagged_overlay_client"), title, "region-editor", str(output)], stdout=log, stderr=subprocess.STDOUT)
    if not wait_for(lambda: title in order_all(), 60):
        raise RuntimeError(f"the {title} window never appeared")
    return process


def order_all():
    return run_script('for (const w of workspace.windowList()) { if (!w.deleted) print("MARK|" + w.caption); }')


def overlay_on_another_output_keeps_focus(checks):
    activate("C")
    overlay = open_overlay("Overlay", 0)
    checks.expect(wait_for(lambda: active_title() == "Overlay", 5), f"the fullscreen unmanaged overlay on the first output is active ({active_title()})")
    checks.expect(not wait_for(lambda: active_title() != "Overlay", 3), f"Konveyor does not take focus back from it ({active_title()})")
    overlay.terminate()


def main():
    Checks().run(clicking_the_active_tiled_window_leaves_floating_on_top, activating_on_one_output_leaves_the_other_alone, overlay_on_another_output_keeps_focus)


if __name__ == "__main__":
    main()
