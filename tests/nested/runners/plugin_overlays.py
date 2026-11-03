#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import (
    CLIENTS,
    activate,
    for_window,
    konveyor_windows,
    managed_titles,
    run_script,
    wait_for,
)


ISSUED = [0]


def next_id():
    ISSUED[0] = max(ISSUED[0], *(window["id"] for window in konveyor_windows())) + 1
    return ISSUED[0]


def launch(client, *arguments):
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    return subprocess.Popen(
        ["qml6", str(CLIENTS / client), "--", *arguments],
        stdout=log,
        stderr=subprocess.STDOUT,
    )


def geometry(title):
    printed = run_script(
        for_window(
            title,
            'const g = w.frameGeometry; print("MARK|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height);',
        )
    )
    return [float(value) for value in printed[0].split("|")] if printed else None


def centred_on(card, host):
    shown, box = geometry(card), geometry(host)
    return bool(shown and box) and shown[0] == round(box[0] + (box[2] - shown[2]) / 2)


def close_window(title):
    run_script(for_window(title, "w.closeWindow();"))


def overlay(target, slot=0):
    return f"Konveyor Monitor Overlay {target} {slot}"


def overlay_naming_itself(checks):
    own = next_id()
    process = launch("retitle-client.qml", "Selfish", overlay(own))
    checks.expect(
        wait_for(lambda: geometry(overlay(own)), 60),
        "a window renamed as an overlay of itself appears",
    )
    checks.expect(
        overlay(own) in managed_titles(),
        "it stays an ordinary window instead of attaching to itself",
    )
    close_window(overlay(own))
    process.wait(timeout=30)


def dialog_becoming_overlay_of_its_own_parent(checks):
    own = next_id()
    process = launch(
        "retitle-dialog-client.qml", "Owner", overlay(own), overlay(own, 1)
    )
    checks.expect(wait_for(lambda: geometry("Owner"), 60), "the owner window opened")
    checks.expect(
        wait_for(lambda: centred_on(overlay(own), "Owner"), 60),
        "a dialog renamed as an overlay of its own parent is centred on it",
    )
    checks.expect(
        wait_for(lambda: centred_on(overlay(own, 1), "Owner"), 60),
        "renaming it again for the same parent keeps it there",
    )
    close_window("Owner")
    process.wait(timeout=30)


def dialog_becoming_overlay_of_another_window(checks):
    activate("A")
    target = next(
        window["id"] for window in konveyor_windows() if window["title"] == "A"
    )
    process = launch(
        "retitle-dialog-client.qml", "Parent", overlay(target), overlay(target, 1)
    )
    checks.expect(
        wait_for(lambda: centred_on(overlay(target), "A"), 60),
        "a dialog renamed as an overlay of a different window follows that window",
    )
    close_window(overlay(target, 1))
    close_window("Parent")
    process.wait(timeout=30)
    checks.expect("A" in managed_titles(), "the window they pointed at is untouched")


def overlay_while_window_toggles_fullscreen(checks):
    target = next(
        window["id"] for window in konveyor_windows() if window["title"] == "B"
    )
    process = launch("overlay-client.qml", overlay(target), "200", "32")
    checks.expect(wait_for(lambda: geometry(overlay(target)), 60), "the overlay opened")
    run_script(for_window("B", "w.fullScreen = true;"))
    checks.expect(
        wait_for(
            lambda: (
                run_script(for_window("B", 'print("MARK|" + w.fullScreen);'))
                == ["true"]
            )
        ),
        "B is fullscreen",
    )
    run_script(for_window("B", "w.fullScreen = false;"))
    checks.expect(
        wait_for(
            lambda: (
                run_script(for_window("B", 'print("MARK|" + w.fullScreen);'))
                == ["false"]
            )
        ),
        "B left fullscreen",
    )
    close_window(overlay(target))
    process.wait(timeout=30)


def main():
    Checks().run(
        overlay_naming_itself,
        dialog_becoming_overlay_of_its_own_parent,
        dialog_becoming_overlay_of_another_window,
        overlay_while_window_toggles_fullscreen,
    )


if __name__ == "__main__":
    main()
