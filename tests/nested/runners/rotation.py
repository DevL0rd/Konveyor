#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import wait_for


def dbus(method, *args):
    return subprocess.run(["qdbus6", "org.kde.Konveyor", "/Konveyor", f"org.kde.Konveyor.{method}", *args],
                          capture_output=True, text=True, check=True).stdout.strip()


def tiles():
    out = {}
    for window in json.loads(dbus("Windows")):
        info = window.get("layout") or {}
        position, size = info.get("tile_pos_in_workspace_view"), info.get("tile_size")
        if position and size:
            out[window.get("title")] = (round(position[0]), round(position[1]), round(size[0]), bool(window.get("is_focused")))
    return out


def output_logical():
    return json.loads(dbus("Outputs"))[0]["logical"]


def rotate(orientation, settled):
    output = json.loads(dbus("Outputs"))[0]["name"]
    subprocess.run(["kscreen-doctor", f"output.{output}.rotation.{orientation}"], capture_output=True, text=True)
    wait_for(lambda: settled(output_logical(), tiles()), 60)
    return output_logical()


def columns(current):
    return len({x for x, _, _, _ in current.values()})


def main():
    problems = []
    before = tiles()
    focused = [title for title, tile in before.items() if tile[3]]
    print(f"landscape: {before}")
    logical = rotate("left", lambda screen, current: screen["width"] < screen["height"]
                     and all(width >= screen["width"] * 0.9 for _, _, width, _ in current.values()))
    portrait = tiles()
    print(f"portrait {logical['width']}x{logical['height']}: {portrait}")
    if logical["width"] >= logical["height"]:
        problems.append(f"the output did not rotate: {logical}")
    if columns(portrait) != 2:
        problems.append(f"expected 2 columns in portrait, got {columns(portrait)}")
    if any(width < logical["width"] * 0.9 for _, _, width, _ in portrait.values()):
        problems.append("portrait columns are not full width")
    if [title for title, tile in portrait.items() if tile[3]] != focused:
        problems.append("focus moved while rotating to portrait")
    widths = sorted(width for _, _, width, _ in before.values())
    rotate("normal", lambda screen, current: sorted(width for _, _, width, _ in current.values()) == widths)
    after = tiles()
    print(f"landscape again: {after}")
    if columns(after) != len(before):
        problems.append(f"expected {len(before)} columns after rotating back, got {columns(after)}")
    if sorted(width for _, _, width, _ in after.values()) != widths:
        problems.append("column widths did not return to the landscape widths")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
