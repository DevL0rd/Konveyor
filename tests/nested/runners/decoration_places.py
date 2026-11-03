#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from borderprobe import MAGENTA, painted_titles, probe_config, screenshot, solid
from checks import Checks, load_config
from fakepointer import move
from kwinsession import activate, active_title, konveyor_action, open_client, wait_for

FIRST = ("A", "B")
SECOND = ("F", "G")
EVERY = FIRST + SECOND


def focus(title):
    activate(title)
    wait_for(lambda: active_title() == title)


def expect_painted(checks, expected, message, timeout=20):
    seen = set()

    def matches():
        nonlocal seen
        seen = painted_titles(screenshot(), EVERY, MAGENTA)
        return seen == set(expected)

    checks.expect(
        wait_for(matches, timeout, 0.3),
        f"{message} (painted {sorted(seen)}, expected {sorted(expected)})",
    )


def play_on_both_outputs(checks):
    move(960, 540, settle=False)
    for title in FIRST:
        open_client(title)
    move(2880, 540, settle=False)
    for title in SECOND:
        open_client(title)
    events = '        on "focus" "Mark" reach="all" duration=30'
    checks.expect(
        load_config(probe_config(events, solid("Mark", "#ff00ff"))),
        "a focus effect reaching every window loads",
    )
    focus("A")
    focus("B")
    expect_painted(
        checks, set(FIRST), "reach all stays on the output of the focused window"
    )
    focus("G")
    expect_painted(checks, set(SECOND), "and follows the focus to the other output")


def fullscreen_covers_only_its_own_output(checks):
    focus("F")
    checks.equal(konveyor_action("fullscreen-window"), "", "make F fullscreen")
    focus("A")
    focus("B")
    expect_painted(
        checks,
        set(FIRST),
        "a fullscreen window on the other output does not stop the effect",
    )
    focus("G")
    checks.expect(
        not painted_titles(screenshot(), EVERY, MAGENTA),
        "a fullscreen window on the same output covers the effect",
    )
    focus("F")
    checks.equal(konveyor_action("fullscreen-window"), "", "leave fullscreen")


def windows_on_other_workspaces_are_left_out(checks):
    focus("B")
    checks.equal(
        konveyor_action("move-window-to-workspace-down"),
        "",
        "move B one workspace down",
    )
    focus("A")
    checks.expect(
        wait_for(lambda: "A" in painted_titles(screenshot(), EVERY, MAGENTA)),
        "the effect plays on A with B on another workspace",
    )
    checks.equal(konveyor_action("focus-workspace-down"), "", "follow B")
    checks.expect(
        wait_for(lambda: active_title() == "B"), "B is focused on its workspace"
    )
    checks.equal(konveyor_action("focus-workspace-up"), "", "and come back")
    focus("A")
    checks.expect(
        wait_for(lambda: "A" in painted_titles(screenshot(), EVERY, MAGENTA)),
        "the effect plays on A again after the round trip",
    )


def main():
    Checks().run(
        play_on_both_outputs,
        fullscreen_covers_only_its_own_output,
        windows_on_other_workspaces_are_left_out,
    )


if __name__ == "__main__":
    main()
