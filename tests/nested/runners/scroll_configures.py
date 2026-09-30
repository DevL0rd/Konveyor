#!/usr/bin/env python3
import json
import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from fakepointer import touch
from kwinsession import active_title, konveyor, konveyor_action, konveyor_windows, run_script, wait_for

LOGS = Path(os.environ["KONVEYOR_CLIENT_LOGS"])
LABELS = "abcdef"
SWIPES = [900, -900, -900, 900]
ALLOWED_PER_SWIPE = 4


def activate(label):
    run_script(f'for (const w of workspace.windowList()) {{ if (!w.deleted && w.caption.startsWith("{label} ")) {{ workspace.activeWindow = w; }} }}')
    wait_for(lambda: (active_title() or "").startswith(f"{label} "), 60)


def labels():
    return [window["title"].split(" ")[0] for window in konveyor_windows()]


def open_browsers(chrome):
    for label in LABELS:
        profile = LOGS / f"chrome-{label}"
        profile.mkdir()
        with open(LOGS / f"{label}.log", "w") as log:
            subprocess.Popen([chrome, "--ozone-platform=wayland", "--no-first-run", "--no-default-browser-check", "--disable-sync",
                              "--password-store=basic", f"--user-data-dir={profile}", f"data:text/html,<title>{label}</title>{label}"],
                             stdout=subprocess.DEVNULL, stderr=log, env=dict(os.environ, WAYLAND_DEBUG="client"))
        if not wait_for(lambda: label in labels(), 120):
            raise RuntimeError(f"browser window {label} never appeared")


def settled():
    frames = dict(line.split("|", 1) for line in run_script(
        'for (const w of workspace.windowList()) { if (!w.deleted && w.normalWindow) print("MARK|" + w.caption + "|" + w.frameGeometry.x + "," + w.frameGeometry.width); }'))
    targets = {window["title"]: (window["layout"]["tile_pos_in_workspace_view"][0], window["layout"]["tile_size"][0]) for window in konveyor_windows()}
    return all(title in frames and all(abs(float(a) - b) < 1 for a, b in zip(frames[title].split(","), target)) for title, target in targets.items())


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
    wait_for(lambda: json.loads(konveyor("Outputs"))[0]["logical"]["scale"] == 1.25, 60)
    open_browsers(sys.argv[1])
    for label, pulls in (("b", 1), ("d", 2)):
        activate(label)
        for _ in range(pulls):
            count = len(columns())
            konveyor_action("consume-window-into-column")
            wait_for(lambda: len(columns()) == count - 1, 60)
    activate("c")
    wait_for(settled, 60)
    layout = columns()
    rows = {label: len(column) for column in layout for _, label, _ in column}
    for column in layout:
        print("column: " + " ".join(f"{label} {size[0]}x{size[1]}" for _, label, size in column))
    for count in (1, 2, 3):
        if count not in rows.values():
            problems.append(f"no column with {count} rows to scroll past")

    before = {label: configures(label) for label in LABELS}
    for dx in SWIPES:
        touch(3, 1100 if dx < 0 else 380, 1300, dx=dx, radius=40, steps=90, settle=False)
        wait_for(settled, 60)
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
