#!/usr/bin/env python3
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from fakepointer import Held, fake, move
from kwinsession import CLIENTS, active_title, for_window, frame, konveyor_action, kwin_titles, open_client, run_script, wait_for


def enable_focus_follows_mouse():
    return load_config(default_config().replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n"))


def occurrences(needle):
    with open(os.environ["KONVEYOR_KWIN_LOG"], errors="replace") as log:
        return sum(needle in line for line in log)


def launch(label, width, height, kind):
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    return subprocess.Popen(["qml6", str(CLIENTS / "dialog-client.qml"), "--", label, str(width), str(height), kind, "1500", "500", "300"],
                            stdout=log, stderr=subprocess.STDOUT)


def place(title, x, y, width, height):
    run_script(for_window(title, f"w.frameGeometry = {{x: {x}, y: {y}, width: {width}, height: {height}}};"))
    return wait_for(lambda: frame(title) == (x, y, width, height), 5)


def reach(waypoints, slow):
    if slow:
        with Held() as held:
            for start, end in zip(waypoints, waypoints[1:]):
                held.glide(start, end, 30)
    else:
        for point in waypoints[1:]:
            move(*point, settle=False)


def click_and_verify(checks, label, title, target, nudge=False):
    before = occurrences(f"konveyor-test-clicked:{title}")
    if nudge:
        move(target[0] + 1, target[1])
    fake("click", *target, settle=0.5)
    checks.expect(wait_for(lambda: occurrences(f"konveyor-test-clicked:{title}") > before, 3), f"{label}: the click reached {title}")
    checks.expect(title in kwin_titles(), f"{label}: {title} is still open")
    checks.expect(active_title() == title, f"{label}: {title} is active after the click ({active_title()})")


def open_dialog(kind, geometry=None, pointer=(1500, 900)):
    move(*pointer)
    host = launch("H", 900, 900, kind)
    wait_for(lambda: active_title() == "H", 30)
    wait_for(lambda: "H-Dialog" in kwin_titles(), 30)
    time.sleep(0.5)
    if geometry:
        place("H-Dialog", *geometry)
    time.sleep(0.5)
    return host


def dialog_case(checks, name, kind, geometry, waypoints, pointer=(1500, 900)):
    for speed in ("slow", "quick"):
        host = open_dialog(kind, geometry=geometry, pointer=pointer)
        try:
            label = f"{name}/{speed}"
            checks.expect(active_title() == "H-Dialog", f"{label}: the dialog is active when it opens ({active_title()})")
            reach([pointer, *waypoints], speed == "slow")
            checks.expect(not wait_for(lambda: active_title() != "H-Dialog", 1.5), f"{label}: hovering the dialog keeps it active ({active_title()})")
            click_and_verify(checks, label, "H-Dialog", waypoints[-1])
        finally:
            host.terminate()
            wait_for(lambda: "H" not in kwin_titles(), 10)


def dialogs_over_two_windows(checks):
    checks.expect(enable_focus_follows_mouse(), "focus mode loads")
    over = (700, 300, 500, 300)
    for kind in ("dialog", "modal"):
        dialog_case(checks, f"{kind} over two tiles, centre", kind, over, [(1100, 450), (950, 450)])
        dialog_case(checks, f"{kind} over two tiles, jittering across", kind, over, [(1100, 400), (800, 500), (1150, 350), (760, 580), (950, 450)])


def dialog_under_stationary_pointer(checks):
    checks.expect(enable_focus_follows_mouse(), "focus mode loads")
    for nudge in (False, True):
        host = open_dialog("dialog", pointer=(1436, 540))
        try:
            checks.expect(active_title() == "H-Dialog", f"stationary: the dialog is active when it opens under the pointer ({active_title()})")
            click_and_verify(checks, f"stationary{'+nudge' if nudge else ''}", "H-Dialog", (1436, 540), nudge)
        finally:
            host.terminate()
            wait_for(lambda: "H" not in kwin_titles(), 10)


def floated_window_over_tiles(checks):
    checks.expect(enable_focus_follows_mouse(), "focus mode loads")
    floated = open_client("F")
    try:
        konveyor_action("toggle-window-floating")
        checks.expect(wait_for(lambda: place("F", 700, 300, 500, 300), 15), "the floated window sits over two tiles")
        for speed in ("slow", "quick"):
            move(300, 900)
            reach([(300, 900), (800, 450), (1100, 450)], speed == "slow")
            checks.expect(wait_for(lambda: active_title() == "F", 3), f"floated/{speed}: hovering the floated window focuses it ({active_title()})")
            click_and_verify(checks, f"floated/{speed}", "F", (1100, 450))
    finally:
        floated.terminate()


def main():
    Checks().run(dialogs_over_two_windows, dialog_under_stationary_pointer, floated_window_over_tiles)


if __name__ == "__main__":
    main()
