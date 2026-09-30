#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import for_window, run_script, wait_for


def dbus(method, *args):
    return subprocess.run(["qdbus6", "org.kde.Konveyor", "/Konveyor", f"org.kde.Konveyor.{method}", *args],
                          capture_output=True, text=True, check=True).stdout.strip()


def action(name, *arguments):
    dbus("Action", json.dumps({"name": name, "arguments": list(arguments), "properties": {}}))
    wait_for(focused_width_committed, 60)


def focused_width_committed():
    focused = next((window for window in json.loads(dbus("Windows")) if window.get("is_focused")), None)
    if not focused:
        return False
    printed = run_script(for_window(focused["title"], 'print("MARK|" + w.frameGeometry.width);'))
    return bool(printed) and abs(float(printed[0]) - focused["layout"]["tile_size"][0]) < 1


def rows():
    out = []
    for window in json.loads(dbus("Windows")):
        info = window.get("layout") or {}
        position, size = info.get("tile_pos_in_workspace_view"), info.get("tile_size")
        if position and size:
            out.append((window.get("app_id"), round(position[0], 1), round(size[0], 1), bool(window.get("is_focused"))))
    return sorted(out, key=lambda r: r[1])


def main():
    width = json.loads(dbus("Outputs"))[0]["logical"]["width"]
    action("focus-column-last")
    problems = []
    for label in ["90%", "25%", "40%", "100%", "33%"]:
        action("set-column-width", label)
        current = rows()
        shape = " | ".join(f"{a}@{x}+{w}{'*' if f else ''}" for a, x, w, f in current)
        right = max(x + w for _, x, w, _ in current)
        left = min(x for _, x, _, _ in current)
        gap = width - right
        print(f"width {label:5}: {shape}   strip right edge {right}, slack {round(gap, 1)}")
        if gap > 17 and left < 0:
            problems.append(f"{label}: {round(gap, 1)}px dead space on the right while windows are cut off on the left")
        focused = [r for r in current if r[3]]
        for app, x, w, _ in focused:
            if x < -0.5 or (x + w > width + 0.5 and w < width):
                problems.append(f"{label}: focused {app} off screen at {x}..{x + w}")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
