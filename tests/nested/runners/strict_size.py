#!/usr/bin/env python3
import os
import subprocess
import sys
import time
from itertools import count
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import CLIENTS, activate, frame, konveyor_action, konveyor_windows, wait_for, window_state
from nested import build_dir

ROOT = Path(os.environ["KONVEYOR_TEST_ROOT"])
OVERRIDE = ROOT / "config" / "konveyor" / "force-resizable.kdl"
DECLARED = (500.0, 400.0)
ids = count()


class Client:
    def __init__(self, rule, mode="", x11=False, app="", size=DECLARED):
        self.title = f"Strict{next(ids)}"
        self.app_id = f"konveyor-strict-{'forced' if rule else 'plain'}-{app or self.title}"
        self.declared = size
        self.log = ROOT / f"{self.title}.log"
        command = ["python3", str(CLIENTS / "x11-fixed-client.py")] if x11 else [str(build_dir() / "bin" / "strict_size_client")]
        self.process = subprocess.Popen([*command, self.title, self.app_id, mode or "-", f"{int(size[0])}x{int(size[1])}"], stdin=subprocess.PIPE, stdout=open(self.log, "w"),
                                        stderr=subprocess.STDOUT, text=True)
        if not wait_for(lambda: self.window() is not None, 30):
            raise RuntimeError(f"{self.title} never appeared")
        activate(self.title)

    def window(self):
        return next((window for window in konveyor_windows() if window["title"] == self.title), None)

    def size(self):
        values = frame(self.title)
        return values[2:4] if values else None

    def layout(self):
        return tuple(float(value) for value in self.window()["layout"]["window_size"])

    def follows_layout(self):
        return wait_for(lambda: self.size() == self.layout() and self.size() != self.declared, 10)

    def resize(self, width, height, declare=True):
        self.process.stdin.write(f"{'resize' if declare else 'request'} {width}x{height}\n")
        self.process.stdin.flush()

    def saw_strict_state(self):
        return "strict=1" in self.log.read_text()

    def forced_off(self):
        if not OVERRIDE.exists():
            return False
        rules = [block for block in OVERRIDE.read_text().split("window-rule") if self.app_id.replace("-", "\\-") in block]
        return bool(rules) and "force-resizable false" in rules[-1]

    def force_off(self):
        activate(self.title)
        for _ in range(10):
            if self.forced_off():
                break
            konveyor_action("switch-preset-column-width-back")
            time.sleep(1)

    def force_on(self):
        activate(self.title)
        konveyor_action("switch-preset-column-width")

    def stop(self):
        self.process.terminate()
        self.process.wait(timeout=10)
        wait_for(lambda: self.window() is None, 10)


def forced_window_takes_the_layout_size(checks, x11):
    client = Client(True, x11=x11)
    checks.expect(client.follows_layout(), f"a force-resizable fixed window that ignores plain sizes follows the layout ({client.size()} vs {client.layout()})")
    checks.expect(x11 or client.saw_strict_state(), "it was given a state it obeys")
    client.stop()


def force_off_returns_to_declared_size(checks, x11):
    client = Client(True, x11=x11)
    client.follows_layout()
    for round_ in range(3):
        client.force_off()
        checks.expect(wait_for(lambda: client.size() == DECLARED, 10), f"round {round_}: force off returns to the size declared at open ({client.size()})")
        checks.equal(client.layout(), DECLARED, f"round {round_}: and the layout agrees")
        client.force_on()
        checks.expect(client.follows_layout(), f"round {round_}: force on follows the layout again ({client.size()} vs {client.layout()})")
    client.stop()


def self_resize_while_forced_becomes_native(checks, x11):
    client = Client(True, x11=x11)
    client.follows_layout()
    client.resize(640, 480, declare=False)
    time.sleep(1)
    checks.expect(client.follows_layout(), f"a forced window that resized itself is put back on the layout size ({client.size()} vs {client.layout()})")
    client.force_off()
    checks.expect(wait_for(lambda: client.size() == (640.0, 480.0), 10), f"force off returns to the size the client chose ({client.size()})")
    client.stop()


def rapid_cycling_with_a_slow_client(checks, x11):
    client = Client(True, "slow", x11=x11)
    client.follows_layout()
    for _ in range(4):
        client.force_on()
        time.sleep(0.05)
    time.sleep(3)
    client.force_off()
    checks.expect(wait_for(lambda: client.size() == DECLARED, 15), f"late answers to old sizes do not change the native size ({client.size()})")
    client.stop()


def unforced_window_keeps_its_size_and_follows_its_own_choice(checks, x11):
    client = Client(False, x11=x11)
    time.sleep(2)
    checks.equal(client.size(), DECLARED, "a fixed window without the rule keeps its own size")
    checks.expect(x11 or not client.saw_strict_state(), "and is never given the strict state")
    client.resize(640, 480)
    checks.expect(wait_for(lambda: client.size() == (640.0, 480.0) and client.layout() == (640.0, 480.0), 10),
                  f"a fixed window that resizes itself while unforced is followed ({client.size()} vs {client.layout()})")
    client.stop()


def ordinary_client_is_untouched(checks):
    process = subprocess.Popen(["qml6", str(CLIENTS / "fixed-client.qml"), "--", "Ordinary"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    wait_for(lambda: any(window["title"] == "Ordinary" for window in konveyor_windows()), 60)
    time.sleep(2)
    checks.equal(frame("Ordinary")[2:4], DECLARED, "an ordinary fixed Wayland client keeps its size")
    process.terminate()
    process.wait(timeout=10)


def fullscreen_still_works(checks, x11):
    client = Client(True, x11=x11)
    client.follows_layout()
    konveyor_action("fullscreen-window")
    checks.expect(wait_for(lambda: (window_state(client.title) or "").startswith("true|"), 10), "a forced window can go fullscreen")
    checks.expect(wait_for(lambda: client.size() == (1920.0, 1080.0), 10), f"and covers the output ({client.size()})")
    konveyor_action("fullscreen-window")
    checks.expect(wait_for(lambda: (window_state(client.title) or "").startswith("false|"), 10), "and leave fullscreen")
    checks.expect(client.follows_layout(), f"back on the layout size ({client.size()} vs {client.layout()})")
    client.stop()


def reopen(rule, x11, app, size):
    client = Client(rule, x11=x11, app=app, size=size)
    client.follows_layout() if rule else time.sleep(2)
    return client


def native_size_is_remembered_per_app(checks, x11):
    suffix = "x" if x11 else "w"
    first = Client(True, x11=x11, app=f"memory-a-{suffix}")
    first.follows_layout()
    first.stop()
    other = Client(True, x11=x11, app=f"memory-b-{suffix}", size=(700.0, 500.0))
    other.follows_layout()
    other.stop()

    reopened = reopen(True, x11, f"memory-a-{suffix}", (800.0, 600.0))
    reopened.force_off()
    checks.expect(wait_for(lambda: reopened.size() == DECLARED, 10), f"reopened forced at a saved forced size, force off returns to the stored size ({reopened.size()})")
    reopened.stop()

    second = reopen(True, x11, f"memory-b-{suffix}", (1000.0, 800.0))
    second.force_off()
    checks.expect(wait_for(lambda: second.size() == (700.0, 500.0), 10), f"a second app keeps its own stored size ({second.size()})")
    second.stop()


def own_choice_updates_the_stored_size(checks, x11):
    app = f"memory-c-{'x' if x11 else 'w'}"
    first = Client(True, x11=x11, app=app)
    first.follows_layout()
    first.resize(640, 480, declare=False)
    first.force_off()
    checks.expect(wait_for(lambda: first.size() == (640.0, 480.0), 10), f"the choice made while forced is taken ({first.size()})")
    first.force_on()
    first.follows_layout()
    first.stop()
    reopened = reopen(True, x11, app, (900.0, 700.0))
    reopened.force_off()
    checks.expect(wait_for(lambda: reopened.size() == (640.0, 480.0), 10), f"the updated size is what a reopen returns to ({reopened.size()})")
    reopened.stop()


def unforced_reopen_takes_the_declared_size(checks, x11):
    app = f"memory-d-{'x' if x11 else 'w'}"
    first = reopen(False, x11, app, DECLARED)
    first.stop()
    reopened = reopen(False, x11, app, (640.0, 480.0))
    checks.equal(reopened.size(), (640.0, 480.0), "without force resizing a reopened app keeps the size it declares now")
    reopened.stop()


def scenario(function, x11):
    def step(checks):
        function(checks, x11)

    step.__name__ = function.__name__ + ("_x11" if x11 else "")
    return step


if __name__ == "__main__":
    wayland_only = (ordinary_client_is_untouched,)
    shared = (forced_window_takes_the_layout_size, force_off_returns_to_declared_size, self_resize_while_forced_becomes_native,
              rapid_cycling_with_a_slow_client, unforced_window_keeps_its_size_and_follows_its_own_choice, fullscreen_still_works,
              native_size_is_remembered_per_app, own_choice_updates_the_stored_size, unforced_reopen_takes_the_declared_size)
    Checks().run(*wayland_only, *(scenario(function, x11) for x11 in (False, True) for function in shared))
