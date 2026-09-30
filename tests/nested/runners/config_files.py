#!/usr/bin/env python3
import json
import stat
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, config_path, notifications
from kwinsession import CLIENTS, activate, konveyor, konveyor_action, konveyor_windows, managed_titles, open_client, wait_for

MODE = sys.argv[1]
MARKER_BINDS = 'binds {\n    Mod+F7 { spawn "true"; }\n}\n'
OVERRIDE = config_path().parent / "force-resizable.kdl"


def bind_keys():
    return sorted(bind["key"] for bind in json.loads(konveyor("Binds")))


def summaries():
    return [entry["summary"] + ": " + entry["body"] for entry in notifications()]


def missing_default(checks):
    checks.expect(wait_for(lambda: any("Could not create" in summary and "bundled default" in summary for summary in summaries())),
                  f"a missing bundled default is reported ({summaries()})")
    checks.expect(wait_for(lambda: any(summary.startswith("Konveyor: failed to load config") for summary in summaries())),
                  "the missing config is reported as a failed load")
    checks.expect(not config_path().exists(), "no config.kdl is made up")
    open_client("A")
    checks.equal(managed_titles(), ["A"], "windows are still tiled with the built-in defaults")
    config_path().write_text(MARKER_BINDS)
    checks.expect(wait_for(lambda: bind_keys() == ["Super+F7"]), f"writing config.kdl afterwards loads it ({bind_keys()})")


def bundled_default(checks):
    checks.expect(wait_for(config_path().exists), "config.kdl is created from the bundled default")
    checks.equal(config_path().read_text(), MARKER_BINDS, "it is a copy of the bundled default")
    checks.equal(stat.S_IMODE(config_path().stat().st_mode), 0o644, "with the usual permissions")
    checks.equal(bind_keys(), ["Super+F7"], "and it is loaded")
    checks.equal(summaries(), [], "without any notification")


def force_resizable(checks):
    subprocess.Popen(["qml6", str(CLIENTS / "fixed-client.qml"), "--", "Fixed"])
    checks.expect(wait_for(lambda: "Fixed" in managed_titles(), 60), "the fixed-size client opened")
    activate("Fixed")
    original = config_path().read_text()
    config_path().chmod(0o444)
    error = konveyor_action("switch-preset-column-width")
    checks.expect("config.kdl" in error, f"cycling the width of a fixed-size window reports the unwritable config ({error!r})")
    checks.expect(not OVERRIDE.exists(), "and leaves no half-written force-resizable.kdl behind")
    checks.equal(config_path().read_text(), original, "config.kdl is unchanged")
    config_path().chmod(0o644)
    before = next(window for window in konveyor_windows() if window["title"] == "Fixed")["layout"]["tile_size"][0]
    checks.equal(konveyor_action("switch-preset-column-width"), "", "cycling the width works once the config is writable")
    checks.expect(OVERRIDE.exists() and "force-resizable true" in OVERRIDE.read_text() and "qml" in OVERRIDE.read_text(),
                  f"force-resizable.kdl holds the rule ({OVERRIDE.read_text() if OVERRIDE.exists() else None!r})")
    checks.expect('include "force-resizable.kdl"' in config_path().read_text(), "config.kdl includes it")
    checks.expect(wait_for(lambda: next(window for window in konveyor_windows() if window["title"] == "Fixed")["layout"]["tile_size"][0] != before),
                  "the window's width changes")
    checks.expect(any("Force resizing enabled" in summary for summary in summaries()), f"a notification says so ({summaries()})")


def main():
    Checks().run({"missing-default": missing_default, "bundled-default": bundled_default, "force-resizable": force_resizable}[MODE])


if __name__ == "__main__":
    main()
