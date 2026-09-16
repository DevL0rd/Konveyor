#!/usr/bin/env python3
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from fakepointer import click, move
from kwinsession import activate, active_title, for_window, konveyor_action, run_script
from screenshot import capture_workspace

CLIENT_COLOR = (0x2F, 0x30, 0x33)


def outputs():
    printed = run_script('for (const o of workspace.screens) { const g = o.geometry; print("MARK|" + o.name + "|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height); }')
    result = {}
    for line in printed:
        name, x, y, width, height = line.split("|")
        result[name] = (float(x), float(y), float(width), float(height))
    return result


def frames():
    printed = run_script('for (const w of workspace.windowList()) { if (!w.deleted && w.normalWindow) { const g = w.frameGeometry; print("MARK|" + w.caption + "|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height); } }')
    result = {}
    for line in printed:
        title, x, y, width, height = line.split("|")
        result[title] = (float(x), float(y), float(width), float(height))
    return result


def intersects(a, b):
    return a[0] < b[0] + b[2] and b[0] < a[0] + a[2] and a[1] < b[1] + b[3] and b[1] < a[1] + a[3]


def client_pixels_in(image, rect):
    left, top, width, height = (int(v) for v in rect)
    count = 0
    for y in range(top, top + height, 7):
        for x in range(left, left + width, 7):
            if image.getpixel((x, y))[:3] == CLIENT_COLOR:
                count += 1
    return count


def check(step, problems):
    screens = outputs()
    home_name, home = sorted(screens.items(), key=lambda item: item[1][0])[0]
    others = {name: rect for name, rect in screens.items() if name != home_name}
    windows = frames()
    print(f"{step}: home={home_name} {home} frames={windows}")
    for title, frame in windows.items():
        for name, rect in others.items():
            if intersects(frame, rect) and not intersects(frame, home):
                problems.append(f"{step}: {title} at {frame} sits entirely on {name}")
    image = capture_workspace(tempfile.mktemp(suffix=".png"))
    scale = image.width / max(r[0] + r[2] for r in screens.values())
    home_shown = client_pixels_in(image, tuple(v * scale for v in home))
    print(f"{step}: client pixels on {home_name}: {home_shown}")
    if not home_shown:
        problems.append(f"{step}: nothing drawn on {home_name}")
    for name, rect in others.items():
        scaled = tuple(v * scale for v in rect)
        shown = client_pixels_in(image, scaled)
        print(f"{step}: client pixels on {name}: {shown}")
        if shown:
            problems.append(f"{step}: {shown} sampled client pixels drawn on {name}")


def check_clicks(problems):
    frame = frames()["B"]
    y = frame[1] + frame[3] / 2
    activate("A")
    time.sleep(1.0)
    click(1940, y)
    after_spill = active_title()
    print(f"click on Virtual-1 over the hidden part of B: active={after_spill}")
    if after_spill != "A":
        problems.append(f"a click on Virtual-1 over the hidden part of B activated {after_spill}")
    click(1900, y)
    after_home = active_title()
    print(f"click on the visible part of B: active={after_home}")
    if after_home != "B":
        problems.append(f"a click on the visible part of B activated {after_home}")
    check_click_reaches_window_below(problems)


def clicks_in(log, label):
    return log.read_text(errors="replace").count(f"konveyor-test-clicked:{label}")


def check_click_reaches_window_below(problems):
    client = Path(__file__).resolve().parent.parent / "clients" / "client.qml"
    move(2880, 540)
    run_script('workspace.activeWindow = null;')
    d_log = Path(tempfile.mktemp(suffix=".log"))
    subprocess.Popen(["qml6", str(client), "--", "D", "700", "500"], stdout=d_log.open("w"), stderr=subprocess.STDOUT)
    time.sleep(4.0)
    d_frame = frames().get("D")
    print(f"D opened at {d_frame}")
    if not d_frame or d_frame[0] < 1920:
        problems.append(f"D did not open on Virtual-1 ({d_frame})")
        return
    activate("B")
    time.sleep(1.0)
    activate("A")
    time.sleep(1.0)
    run_script(for_window("B", "workspace.raiseWindow(w);"))
    order = run_script('for (const w of workspace.stackingOrder) { if (w.normalWindow) print("MARK|" + w.caption); }')
    print(f"stacking bottom to top: {order}")
    if order.index("B") < order.index("D"):
        problems.append(f"B is not stacked above D ({order})")
    b_frame = frames()["B"]
    y = d_frame[1] + d_frame[3] / 2
    print(f"B at {b_frame}, clicking Virtual-1 at 1940,{y}")
    b_clicks = clicks_in(Path(os.environ["KONVEYOR_KWIN_LOG"]), "B")
    click(1940, y)
    d_received = clicks_in(d_log, "D")
    b_received = clicks_in(Path(os.environ["KONVEYOR_KWIN_LOG"]), "B") - b_clicks
    print(f"click on D under the hidden part of B: D received {d_received}, B received {b_received}")
    if d_received != 1 or b_received:
        problems.append(f"a click on D under the hidden part of B reached D {d_received} times and B {b_received} times")


def main():
    problems = []
    if len(outputs()) < 2:
        print("expected two outputs")
        print("RESULT: FAIL")
        return
    for title in ("A", "C", "B", "A"):
        activate(title)
        time.sleep(1.5)
        check(f"{title} activated", problems)
    konveyor_action("set-column-width", "70%")
    time.sleep(1.5)
    check("A widened so B straddles the edge", problems)
    check_clicks(problems)
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
