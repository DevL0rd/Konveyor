#!/usr/bin/env python3
import os
import re
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from fakepointer import touch
from kwinsession import konveyor_action, konveyor_windows, run_script

LOGS = Path(os.environ["KONVEYOR_CLIENT_LOGS"])
LABELS = "abcdef"
SWIPES = [900, -900, -900, 900]
ALLOWED_PER_SWIPE = 4


def activate(label):
    run_script(f'for (const w of workspace.windowList()) {{ if (!w.deleted && w.caption.startsWith("{label} ")) {{ workspace.activeWindow = w; }} }}')
    time.sleep(0.6)


def columns():
    placed = {}
    for window in konveyor_windows():
        layout = window.get("layout") or {}
        position = layout.get("pos_in_scrolling_layout")
        if position:
            placed.setdefault(position[0], []).append((position[1], window["title"].split(" ")[0], tuple(layout.get("tile_size") or ())))
    return [sorted(rows) for _, rows in sorted(placed.items())]


def configures(label):
    return len(re.findall(r"xdg_toplevel#\d+\.configure\(", (LOGS / f"{label}.log").read_text(errors="replace")))


def main():
    problems = []
    for label, pulls in (("b", 1), ("d", 2)):
        activate(label)
        for _ in range(pulls):
            konveyor_action("consume-window-into-column")
            time.sleep(1.5)
    activate("c")
    time.sleep(2)
    layout = columns()
    rows = {label: len(column) for column in layout for _, label, _ in column}
    for column in layout:
        print("column: " + " ".join(f"{label} {size[0]}x{size[1]}" for _, label, size in column))
    for count in (1, 2, 3):
        if count not in rows.values():
            problems.append(f"no column with {count} rows to scroll past")

    before = {label: configures(label) for label in LABELS}
    for dx in SWIPES:
        touch(3, 1100 if dx < 0 else 380, 1300, dx=dx, radius=40, steps=90)
        time.sleep(2)
    after = columns()
    if after != layout:
        problems.append(f"the layout changed while scrolling: {layout} -> {after}")

    for label in LABELS:
        sent = configures(label) - before[label]
        print(f"window {label} in a {rows.get(label)}-row column: {sent} configures during {len(SWIPES)} swipes")
        if sent > ALLOWED_PER_SWIPE * len(SWIPES):
            problems.append(f"{label} ({rows.get(label)} rows) got {sent} configures while the row only scrolled")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
