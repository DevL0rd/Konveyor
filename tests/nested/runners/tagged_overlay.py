#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import frame, kwin_titles, managed_titles, open_client, wait_for
from nested import build_dir


def open_overlay(title, tag):
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    process = subprocess.Popen([str(build_dir() / "bin" / "tagged_overlay_client"), title, tag], stdout=log, stderr=subprocess.STDOUT)
    if not wait_for(lambda: title in kwin_titles(), 60):
        raise RuntimeError(f"the {title} window never appeared")
    return process


def region_editor_overlay_stays_out_of_the_layout(checks):
    open_client("Target", 800, 600)
    before = frame("Target")
    overlay = open_overlay("Overlay", "region-editor")
    checks.expect(wait_for(lambda: "Overlay" in kwin_titles()), "the tagged overlay mapped")
    checks.equal(managed_titles(), ["Target"], "a window tagged region-editor is not tiled")
    checks.equal(frame("Target"), before, "the window being recorded keeps its place")
    overlay.terminate()


def other_tags_are_tiled(checks):
    overlay = open_overlay("Plain", "something-else")
    checks.expect(wait_for(lambda: "Plain" in managed_titles()), "a window with any other tag is tiled")
    overlay.terminate()


def main():
    Checks().run(region_editor_overlay_stays_out_of_the_layout, other_tags_are_tiled)


if __name__ == "__main__":
    main()
