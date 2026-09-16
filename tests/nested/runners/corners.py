#!/usr/bin/env python3
import json
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from screenshot import capture_workspace


def windows():
    output = subprocess.run(["qdbus6", "org.kde.Konveyor", "/Konveyor", "org.kde.Konveyor.Windows"],
                            capture_output=True, text=True, check=True).stdout
    return json.loads(output)


def main():
    image = capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).with_suffix(".png")))
    problems = []
    checked = 0
    for window in windows():
        layout = window.get("layout") or {}
        position, size = layout.get("tile_pos_in_workspace_view"), layout.get("tile_size")
        if not position or not size or position[0] < 0 or position[0] + size[0] > image.width:
            continue
        x, y, width, height = (round(v) for v in (*position, *size))
        corner = image.getpixel((x + 1, y + 1))
        center = image.getpixel((x + width // 2, y + height // 2))
        checked += 1
        print(f"{window.get('title')}: corner={corner} center={center}")
        if corner[3] != 0:
            problems.append(f"{window.get('title')}: corner pixel is not clipped")
        if center[3] != 255:
            problems.append(f"{window.get('title')}: window content is missing")
    if checked == 0:
        problems.append("no window was fully on screen")
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
