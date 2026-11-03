#!/usr/bin/env python3
import json
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, notifications
from kwinsession import (
    CLIENTS,
    activate,
    for_window,
    konveyor,
    konveyor_action,
    konveyor_windows,
    managed_titles,
    open_client,
    qdbus,
    run_script,
    wait_for,
    window_state,
)


def overview_state():
    return json.loads(konveyor("OverviewState"))["is_open"]


def window_id(title):
    return next(
        window["id"] for window in konveyor_windows() if window["title"] == title
    )


def action_for(name, window, *arguments):
    return konveyor(
        "Action",
        json.dumps(
            {"name": name, "arguments": list(arguments), "properties": {}, "id": window}
        ),
    )


def overview_unavailable(checks):
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.unloadEffect", "overview")
    for name in ("toggle-overview", "open-overview", "close-overview"):
        checks.equal(
            konveyor_action(name),
            "",
            f"{name} without KDE's Overview effect is not an error",
        )
    checks.equal(overview_state(), False, "the overview stays closed")
    checks.equal(konveyor_action("close-overview"), "", "closing it again")


def app_rules_for_missing_windows(checks):
    checks.equal(
        konveyor("AppRules", json.dumps({"id": 99999})),
        "null",
        "AppRules for a window that does not exist",
    )
    checks.expect(
        "no window with id 99999"
        in konveyor("SetAppRule", json.dumps({"id": 99999, "option": "floating"})),
        "SetAppRule for a window that does not exist",
    )


def app_rules_for_stacked_and_floating(checks):
    first = window_id("A")
    activate("A")
    checks.equal(konveyor_action("consume-window-into-column"), "", "stack B under A")
    stacked = json.loads(konveyor("AppRules", json.dumps({"id": first})))
    checks.expect(
        stacked["height"] is not None, f"a stacked tile reports its height ({stacked})"
    )
    checks.equal(action_for("toggle-window-floating", first), "", "float A")
    floating = json.loads(konveyor("AppRules", json.dumps({"id": first})))
    checks.expect(
        floating["column"] is None and floating["height"] is not None,
        f"a floating window reports its height and no column ({floating})",
    )
    checks.equal(action_for("toggle-window-floating", first), "", "tile A again")


def spawn_overlay(title, width=200, height=32):
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    return subprocess.Popen(
        [
            "qml6",
            str(CLIENTS / "overlay-client.qml"),
            "--",
            title,
            str(width),
            str(height),
        ],
        stdout=log,
        stderr=subprocess.STDOUT,
    )


def kwin_geometry(title):
    printed = run_script(
        for_window(
            title,
            'const g = w.frameGeometry; print("MARK|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height);',
        )
    )
    return [float(value) for value in printed[0].split("|")] if printed else None


def close_window(title):
    run_script(for_window(title, "w.closeWindow();"))


def monitor_overlays_without_their_target(checks):
    target = window_id("A")
    ghost = spawn_overlay("Konveyor Monitor Overlay 99999 0")
    first = spawn_overlay(f"Konveyor Monitor Overlay {target} 0", 200)
    second = spawn_overlay(f"Konveyor Monitor Overlay {target} 1", 100)
    panel = spawn_overlay(f"Konveyor Monitor Panel {target} 1", 300, 200)
    names = [
        f"Konveyor Monitor Overlay {target} 0",
        f"Konveyor Monitor Overlay {target} 1",
        f"Konveyor Monitor Panel {target} 1",
    ]
    checks.expect(
        wait_for(lambda: all(kwin_geometry(name) for name in names), 60),
        "the overlay windows appeared",
    )
    activate("A")

    def centred(name, width):
        host = kwin_geometry("A")
        return kwin_geometry(name)[0] == round(host[0] + (host[2] - width) / 2)

    checks.expect(
        wait_for(lambda: centred(names[0], 300)),
        "both cards are centred on their window",
    )
    close_window(names[0])
    checks.expect(
        wait_for(lambda: centred(names[1], 100)),
        "closing one card re-centres the other",
    )
    close_window(names[2])
    close_window(names[1])
    close_window("Konveyor Monitor Overlay 99999 0")
    for process in (ghost, first, second, panel):
        process.wait(timeout=30)
    checks.expect("A" in managed_titles(), "the window they belonged to is untouched")


def monitor_overlays_that_change_owner(checks):
    first, second = window_id("A"), window_id("B")
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    moving = subprocess.Popen(
        [
            "qml6",
            str(CLIENTS / "retitle-client.qml"),
            "--",
            f"Konveyor Monitor Overlay {first} 0",
            f"Konveyor Monitor Overlay {second} 0",
        ],
        stdout=log,
        stderr=subprocess.STDOUT,
    )
    sliding = subprocess.Popen(
        [
            "qml6",
            str(CLIENTS / "retitle-client.qml"),
            "--",
            f"Konveyor Monitor Panel {first} 1",
            f"Konveyor Monitor Panel {first} 2",
        ],
        stdout=log,
        stderr=subprocess.STDOUT,
    )
    activate("B")

    def centred(name, host, width):
        geometry = kwin_geometry(name)
        box = kwin_geometry(host)
        return geometry and box and geometry[0] == round(box[0] + (box[2] - width) / 2)

    moved = f"Konveyor Monitor Overlay {second} 0"
    checks.expect(
        wait_for(lambda: centred(moved, "B", 200), 60),
        "an overlay that is renamed for another window follows that window",
    )
    close_window(moved)
    close_window(f"Konveyor Monitor Panel {first} 2")
    for process in (moving, sliding):
        process.wait(timeout=30)


def summaries():
    return [entry["summary"] for entry in notifications()]


def fixed_window_width():
    return next(window for window in konveyor_windows() if window["title"] == "Fixed")[
        "layout"
    ]["tile_size"][0]


def fixed_size_window_cycling(checks):
    subprocess.Popen(["qml6", str(CLIENTS / "fixed-client.qml"), "--", "Fixed"])
    checks.expect(
        wait_for(lambda: "Fixed" in managed_titles(), 60),
        "the fixed-size client opened",
    )
    activate("Fixed")
    checks.equal(
        konveyor_action("fullscreen-window"),
        "",
        "make the fixed-size window fullscreen",
    )
    checks.expect(
        wait_for(lambda: (window_state("Fixed") or "").startswith("true")),
        "it is fullscreen",
    )
    checks.equal(
        konveyor_action("switch-preset-column-width"),
        "",
        "cycling the width of a fullscreen fixed-size window does nothing",
    )
    checks.equal(konveyor_action("fullscreen-window"), "", "leave fullscreen")
    checks.expect(
        wait_for(lambda: (window_state("Fixed") or "").startswith("false")),
        "it left fullscreen",
    )
    checks.equal(
        konveyor_action("switch-preset-column-width"),
        "",
        "cycling the width forces it resizable",
    )
    checks.expect(
        wait_for(lambda: "Force resizing enabled" in summaries()),
        f"a notification says so ({summaries()})",
    )
    seen = {round(fixed_window_width())}
    for _ in range(8):
        if "Force resizing disabled" in summaries():
            break
        checks.equal(
            konveyor_action("switch-preset-column-width"), "", "cycle forwards"
        )
        wait_for(lambda: round(fixed_window_width()) not in seen, 5)
        seen.add(round(fixed_window_width()))
    checks.expect(
        "Force resizing disabled" in summaries(),
        f"cycling back to the native width turns force resizing off ({summaries()})",
    )
    checks.equal(
        konveyor_action("switch-preset-column-width-back"),
        "",
        "cycling backwards forces it resizable again",
    )
    checks.expect(
        wait_for(lambda: summaries().count("Force resizing enabled") == 2),
        f"and says so ({summaries()})",
    )
    checks.equal(
        konveyor_action("switch-preset-column-width-back"), "", "cycle backwards"
    )
    checks.equal(
        konveyor_action("switch-preset-column-width-back"),
        "",
        "cycle backwards until the native width",
    )
    run_script(for_window("Fixed", "w.closeWindow();"))
    checks.expect(
        wait_for(lambda: "Fixed" not in managed_titles()),
        "the fixed-size window closed",
    )


def fixed_window_cycled_by_id(checks):
    subprocess.Popen(["qml6", str(CLIENTS / "fixed-client.qml"), "--", "ById"])
    checks.expect(wait_for(lambda: "ById" in managed_titles(), 60), "the fixed-size client opened")
    checks.equal(action_for("switch-preset-column-width", window_id("ById")), "", "cycling the width of a window by its id")
    run_script(for_window("ById", "w.closeWindow();"))
    checks.expect(wait_for(lambda: "ById" not in managed_titles()), "the window closed")


def cycling_without_a_focused_window(checks):
    checks.equal(
        konveyor_action("focus-workspace", "2"), "", "go to an empty workspace"
    )
    checks.equal(
        konveyor_action("switch-preset-column-width"),
        "",
        "cycling the width with nothing focused is quiet",
    )
    checks.equal(konveyor_action("focus-workspace", "1"), "", "come back")
    activate("A")
    checks.equal(
        konveyor_action("switch-preset-column-width"),
        "",
        "cycling the width of a resizable window",
    )


def maximize_request(checks):
    activate("A")
    before = window_width("A")
    run_script(for_window("A", "w.setMaximize(true, true);"))
    checks.expect(
        wait_for(lambda: window_width("A") != before),
        f"a maximize request fills the width ({before} -> {window_width('A')})",
    )
    run_script(for_window("A", "w.setMaximize(false, false);"))


def window_width(title):
    return next(window for window in konveyor_windows() if window["title"] == title)[
        "layout"
    ]["tile_size"][0]


def windows_on_every_workspace(checks):
    checks.equal(
        konveyor(
            "SetAppRule",
            json.dumps(
                {"id": window_id("A"), "option": "all-workspaces", "enabled": True}
            ),
        ),
        "",
        "make the app show on every workspace",
    )
    open_client("D")
    shown = run_script(for_window("D", 'print("MARK|" + w.onAllDesktops);'))
    checks.equal(shown, ["true"], "a new window of that app shows on every desktop")
    checks.equal(
        konveyor(
            "SetAppRule",
            json.dumps(
                {"id": window_id("A"), "option": "all-workspaces", "enabled": False}
            ),
        ),
        "",
        "undo the rule",
    )
    close_window("D")


def main():
    Checks().run(
        app_rules_for_missing_windows,
        app_rules_for_stacked_and_floating,
        monitor_overlays_without_their_target,
        monitor_overlays_that_change_owner,
        maximize_request,
        windows_on_every_workspace,
        cycling_without_a_focused_window,
        fixed_size_window_cycling,
        fixed_window_cycled_by_id,
        overview_unavailable,
    )


if __name__ == "__main__":
    main()
