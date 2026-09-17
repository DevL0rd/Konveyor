#!/usr/bin/env python3
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from fakepointer import click
from kwinsession import active_title, konveyor_action, konveyor_windows, run_script
from screenshot import capture_workspace


def keyboard(method, *args):
    return subprocess.run(["busctl", "--user", method, "org.kde.KWin", "/VirtualKeyboard", "org.kde.kwin.VirtualKeyboard", *args],
                          capture_output=True, text=True).stdout.strip()


def frames():
    placed = {}
    for window in konveyor_windows():
        printed = run_script(f'for (const w of workspace.windowList()) {{ if (w.caption == "{window["title"]}") {{ const g = w.frameGeometry; print("MARK|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height); }} }}')
        if printed:
            placed[window["title"]] = tuple(float(v) for v in printed[0].split("|"))
    return placed


def panel_top():
    printed = run_script('for (const w of workspace.stackingOrder) { if (w.inputMethod) { print("MARK|" + w.frameGeometry.y + "|" + w.frameGeometry.height); } }')
    return tuple(float(v) for v in printed[0].split("|")) if printed else None


def main():
    problems = []
    konveyor_action("focus-column-last")
    konveyor_action("consume-or-expel-window-left")
    time.sleep(1.5)
    before = frames()
    print("two rows:", before)
    x, y, width, height = before["C"]
    click(x + width / 2, y + height - 35)
    keyboard("call", "forceActivate")
    time.sleep(3)
    panel = panel_top()
    shown = frames()
    capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).with_suffix(".png")))
    print(f"keyboard visible {keyboard('get-property', 'visible')}, panel {panel}, active {active_title()}")
    print("with keyboard:", shown)
    if panel is None:
        problems.append("the on-screen keyboard did not show")
    else:
        for title in ("B", "C"):
            fx, fy, fw, fh = shown[title]
            if fy + fh > panel[0] + 1:
                problems.append(f"{title} reaches {fy + fh}, under the keyboard at {panel[0]}")
        if shown["C"][1] < shown["B"][1] + shown["B"][3]:
            problems.append(f"C overlaps B in the column ({shown})")
    if panel is not None:
        click(98, panel[0] + panel[1] * 0.13)
        with open(os.environ["KONVEYOR_KWIN_LOG"], errors="replace") as log:
            typed = [line.strip() for line in log if "konveyor-test-typed:" in line]
        print("typed:", typed)
        if not any(line.endswith("C:q") or line.endswith("C:Q") for line in typed):
            problems.append(f"tapping Q on the keyboard did not type into C ({typed})")
    if active_title() != "C":
        problems.append(f"C lost focus while the keyboard opened (active: {active_title()})")
    keyboard("set-property", "active", "b", "false")
    time.sleep(2.5)
    after = frames()
    print("keyboard closed:", after)
    for title in ("B", "C"):
        if abs(after[title][3] - before[title][3]) > 1 or abs(after[title][1] - before[title][1]) > 1:
            problems.append(f"{title} did not return to {before[title]} after the keyboard closed ({after[title]})")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
