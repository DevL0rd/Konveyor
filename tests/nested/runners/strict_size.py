#!/usr/bin/env python3
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import CLIENTS, activate, frame, konveyor_action, konveyor_windows, wait_for, window_state
from nested import build_dir

ROOT = Path(os.environ["KONVEYOR_TEST_ROOT"])
FORCED = "konveyor-strict-forced"
PLAIN = "konveyor-strict-plain"


def start(title, app_id):
    log = open(ROOT / f"{title}.log", "w")
    process = subprocess.Popen([str(build_dir() / "bin" / "strict_size_client"), title, app_id], stdout=log, stderr=subprocess.STDOUT)
    if not wait_for(lambda: any(window["title"] == title for window in konveyor_windows()), 30):
        raise RuntimeError(f"{title} never appeared")
    return process


def layout_size(title):
    window = next(window for window in konveyor_windows() if window["title"] == title)
    return tuple(float(value) for value in window["layout"]["window_size"])


def size_of(title):
    values = frame(title)
    return values[2:4] if values else None


def sizes_settle_on_layout(title):
    return wait_for(lambda: size_of(title) == layout_size(title), 10)


def saw_strict_state(title):
    return "strict=1" in (ROOT / f"{title}.log").read_text()


def stop(process, title):
    process.terminate()
    process.wait(timeout=10)
    wait_for(lambda: all(window["title"] != title for window in konveyor_windows()), 10)


def forced_window_takes_the_layout_size(checks):
    process = start("Forced", FORCED)
    activate("Forced")
    checks.expect(sizes_settle_on_layout("Forced"), f"a force-resizable fixed window that ignores plain sizes follows the layout ({size_of('Forced')} vs {layout_size('Forced')})")
    checks.expect(size_of("Forced") != (500.0, 400.0), "and it really changed from its own size")
    checks.expect(saw_strict_state("Forced"), "it was given a state it obeys")
    stop(process, "Forced")


def unforced_window_keeps_its_size(checks):
    process = start("Plain", PLAIN)
    activate("Plain")
    time.sleep(2)
    checks.equal(size_of("Plain"), (500.0, 400.0), "a fixed window without the rule keeps its own size")
    checks.expect(not saw_strict_state("Plain"), "and is never given the strict state")
    stop(process, "Plain")


def ordinary_client_is_untouched(checks):
    process = subprocess.Popen(["qml6", str(CLIENTS / "fixed-client.qml"), "--", "Ordinary"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    wait_for(lambda: any(window["title"] == "Ordinary" for window in konveyor_windows()), 60)
    time.sleep(2)
    checks.equal(size_of("Ordinary"), (500.0, 400.0), "an ordinary fixed Wayland client keeps its size")
    process.terminate()
    process.wait(timeout=10)


def fullscreen_still_works(checks):
    process = start("Forced", FORCED)
    activate("Forced")
    sizes_settle_on_layout("Forced")
    konveyor_action("fullscreen-window")
    checks.expect(wait_for(lambda: (window_state("Forced") or "").startswith("true|"), 10), "a forced window can go fullscreen")
    checks.expect(wait_for(lambda: size_of("Forced") == (1920.0, 1080.0), 10), f"and covers the output ({size_of('Forced')})")
    konveyor_action("fullscreen-window")
    checks.expect(wait_for(lambda: (window_state("Forced") or "").startswith("false|"), 10), "and leave fullscreen")
    checks.expect(sizes_settle_on_layout("Forced"), f"back on the layout size ({size_of('Forced')} vs {layout_size('Forced')})")
    stop(process, "Forced")


if __name__ == "__main__":
    Checks().run(forced_window_takes_the_layout_size, unforced_window_keeps_its_size, ordinary_client_is_untouched, fullscreen_still_works)
