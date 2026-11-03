#!/usr/bin/env python3
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, notifications
from fakepointer import touch
from kwinsession import (
    for_window,
    konveyor_action,
    managed_titles,
    open_client,
    run_script,
    wait_for,
)

BIN = Path(os.environ["KONVEYOR_FAKE_BIN"])


def install(name, text):
    path = BIN / name
    path.write_text(text)
    path.chmod(0o755)


def four_finger_tap():
    touch(4, 960, 540, radius=60, steps=1, settle=False)


def summaries():
    return [entry["summary"] for entry in notifications()]


def missing_tool(checks):
    four_finger_tap()
    checks.expect(
        wait_for(lambda: "Konveyor: could not open portal-launcher" in summaries()),
        f"a tap that opens a tool that is not installed says so ({summaries()})",
    )


def started_tool(checks):
    install("portal-launcher", f'#!/bin/sh\necho "$@" > "{BIN}/launched"\n')
    four_finger_tap()
    checks.expect(
        wait_for(
            lambda: (
                (BIN / "launched").exists()
                and (BIN / "launched").read_text() == "toggle\n"
            )
        ),
        "the tap starts the tool with the toggle argument",
    )


def overlay_tool_that_is_missing(checks):
    open_client("D")
    run_script(for_window("D", "w.closeWindow();"))
    checks.expect(
        wait_for(lambda: "D" not in managed_titles()),
        "closing a window without the overlay tool installed is quiet",
    )


def overlay_tool_that_runs(checks):
    install("monitor-overlay", f'#!/bin/sh\necho "$@" >> "{BIN}/overlay-calls"\n')
    open_client("D")
    run_script(for_window("D", "w.closeWindow();"))
    checks.expect(
        wait_for(
            lambda: (
                (BIN / "overlay-calls").exists()
                and "remove" in (BIN / "overlay-calls").read_text()
            )
        ),
        "an installed overlay tool is told to remove the overlay of a closed window",
    )


def overlay_tool_that_cannot_run(checks):
    install("monitor-overlay", "#!/nonexistent/interpreter\n")
    open_client("D")
    run_script(for_window("D", "w.closeWindow();"))
    log = Path(os.environ["KONVEYOR_KWIN_LOG"])
    checks.expect(
        wait_for(lambda: "Could not start" in log.read_text(errors="replace")),
        "an installed overlay tool that cannot start is reported when a window closes",
    )


def hotkey_overlay(checks):
    checks.equal(
        konveyor_action("show-hotkey-overlay"),
        "",
        "the hotkey overlay action is accepted",
    )


def main():
    Checks().run(
        missing_tool,
        started_tool,
        overlay_tool_that_is_missing,
        overlay_tool_that_runs,
        overlay_tool_that_cannot_run,
        hotkey_overlay,
    )


if __name__ == "__main__":
    main()
