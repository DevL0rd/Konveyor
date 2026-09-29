#!/usr/bin/env python3
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
SCRIPT = REPO / "widgets" / "portals" / "bin" / "portal-launcher"
SERVICE = ["--user", "call", "org.devl0rd.KontrolPanel", "/KontrolPanel", "org.devl0rd.KontrolPanel"]
STUB = """#!/usr/bin/env python3
import json, os, sys
with open(os.environ["STUB_LOG"], "a") as log:
    log.write(json.dumps([os.path.basename(sys.argv[0])] + sys.argv[1:]) + "\\n")
if os.path.basename(sys.argv[0]) == "busctl" and os.environ.get("STUB_FAIL"):
    sys.stderr.write(os.environ["STUB_FAIL"] + "\\n")
    sys.exit(1)
"""


class TestPortalLauncher(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        root = Path(self.temporary.name)
        stubs = root / "bin"
        stubs.mkdir()
        for tool in ("busctl", "gdbus"):
            (stubs / tool).write_text(STUB)
            (stubs / tool).chmod(0o755)
        self.log = root / "calls"
        self.data = root / "data"
        (self.data / "applications").mkdir(parents=True)
        self.environment = {**os.environ, "PATH": f"{stubs}:{os.environ['PATH']}", "STUB_LOG": str(self.log),
                            "XDG_DATA_HOME": str(self.data), "XDG_DATA_DIRS": str(root / "none")}

    def tearDown(self):
        self.temporary.cleanup()

    def run_launcher(self, *arguments, fail=None):
        environment = dict(self.environment)
        if fail:
            environment["STUB_FAIL"] = fail
        return subprocess.run([sys.executable, str(SCRIPT), *arguments], env=environment, capture_output=True, text=True)

    def calls(self):
        return [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []

    def test_toggle_asks_the_service(self):
        self.assertEqual(self.run_launcher("toggle").returncode, 0)
        self.assertEqual(self.calls(), [["busctl", *SERVICE, "Toggle"]])

    def test_page_opens_the_service_on_that_page(self):
        self.assertEqual(self.run_launcher("games").returncode, 0)
        self.assertEqual(self.calls(), [["busctl", *SERVICE, "Open", "s", "games"]])

    def test_pin_sends_every_file(self):
        self.assertEqual(self.run_launcher("pin", "/a.desktop", "/b c.desktop").returncode, 0)
        self.assertEqual(self.calls(), [["busctl", *SERVICE, "Pin", "as", "2", "/a.desktop", "/b c.desktop"]])

    def test_unknown_requests_print_usage(self):
        for arguments in ([], ["nowhere"], ["pin"], ["add-to", "floor", "x"]):
            result = self.run_launcher(*arguments)
            self.assertEqual(result.returncode, 2)
            self.assertIn("Usage:", result.stderr)
        self.assertEqual(self.calls(), [])

    def test_a_missing_service_fails_visibly(self):
        result = self.run_launcher("toggle", fail="The name org.devl0rd.KontrolPanel was not provided")
        self.assertEqual(result.returncode, 1)
        self.assertIn("was not provided", result.stderr)
        notification = self.calls()[-1]
        self.assertEqual(notification[0], "gdbus")
        self.assertIn("Could not open the Kontrol Panel", notification)
        self.assertIn("The name org.devl0rd.KontrolPanel was not provided", notification)

    def added_script(self):
        call = self.calls()[0]
        self.assertEqual(call[:6], ["busctl", "--user", "call", "org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell"])
        self.assertEqual(call[6:8], ["evaluateScript", "s"])
        return call[8]

    def test_add_to_panel_places_an_icon_for_the_application(self):
        desktop = self.data / "applications" / "org.kde.konsole.desktop"
        desktop.write_text("[Desktop Entry]\n")
        self.assertEqual(self.run_launcher("add-to", "panel", "applications:org.kde.konsole.desktop").returncode, 0)
        script = self.added_script()
        self.assertIn('if ("panel" === "desktop")', script)
        self.assertIn('addWidget("org.kde.plasma.icon")', script)
        self.assertIn('writeConfig("url", "file://%s")' % desktop, script)

    def test_add_to_desktop_accepts_a_path(self):
        desktop = self.data / "applications" / "my app.desktop"
        desktop.write_text("[Desktop Entry]\n")
        self.assertEqual(self.run_launcher("add-to", "desktop", str(desktop)).returncode, 0)
        script = self.added_script()
        self.assertIn('if ("desktop" === "desktop")', script)
        self.assertIn(json.dumps("file://%s" % desktop), script)

    def test_add_to_reports_a_missing_application(self):
        result = self.run_launcher("add-to", "panel", "missing.desktop")
        self.assertEqual(result.returncode, 1)
        self.assertIn("no application named missing.desktop", result.stderr)
        self.assertEqual([call[0] for call in self.calls()], ["gdbus"])
        self.assertIn("Could not add the launcher", self.calls()[0])


if __name__ == "__main__":
    unittest.main()
