#!/usr/bin/env python3
import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HARNESS = """
set -euo pipefail
say() { printf '%s\\n' "$*"; }
die() { printf 'error: %s\\n' "$*" >&2; exit 1; }
WIDGETS_DIR=$1
source "$WIDGETS_DIR/lib.sh"
shift
for step in "$@"; do
    $step
done
"""
STUB = """#!/usr/bin/env python3
import json, os, sys
with open(os.environ["STUB_LOG"], "a") as log:
    log.write(json.dumps([os.path.basename(sys.argv[0])] + sys.argv[1:]) + "\\n")
if os.path.basename(sys.argv[0]) == "busctl" and "shortcut" in sys.argv:
    print(os.environ.get("STUB_SHORTCUT", "ai 0"))
"""
LAUNCHER = ["plasmashell", "activate application launcher", "plasmashell", "Activate Application Launcher"]
KGLOBALACCEL = ["busctl", "--user", "call", "org.kde.kglobalaccel", "/kglobalaccel", "org.kde.KGlobalAccel"]


class TestKontrolPanelInstall(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        root = Path(self.temporary.name)
        self.widgets = root / "widgets"
        shutil.copytree(REPO / "widgets", self.widgets, ignore=shutil.ignore_patterns(".git", "__pycache__"))
        self.home = root / "home"
        self.home.mkdir()
        self.stubs = root / "bin"
        self.stubs.mkdir()
        for tool in ("busctl", "systemctl", "kbuildsycoca6", "konveyor-kontrol-panel"):
            (self.stubs / tool).write_text(STUB)
            (self.stubs / tool).chmod(0o755)
        self.log = root / "calls"
        self.environment = {
            "PATH": f"{self.stubs}:/usr/bin:/bin",
            "HOME": str(self.home),
            "XDG_CONFIG_HOME": str(self.home / ".config"),
            "XDG_DATA_HOME": str(self.home / ".local" / "share"),
            "XDG_STATE_HOME": str(self.home / ".local" / "state"),
            "STUB_LOG": str(self.log),
            "STUB_SHORTCUT": "ai 3 150994992 16777250 268435527",
        }
        self.state = self.home / ".local" / "state" / "konveyor" / "launcher-keys"

    def tearDown(self):
        self.temporary.cleanup()

    def run_steps(self, *steps):
        return subprocess.run(["bash", "-c", HARNESS, "harness", str(self.widgets), *steps],
                              env=self.environment, capture_output=True, text=True)

    def calls(self):
        calls = [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []
        self.log.unlink(missing_ok=True)
        return calls

    def test_takes_meta_and_alt_f1_from_the_plasma_launcher_once(self):
        result = self.run_steps("take_launcher_keys")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn([*KGLOBALACCEL, "setForeignShortcut", "asai", "4", *LAUNCHER, "1", "268435527"], self.calls())
        self.assertEqual(self.state.read_text(), "ai 3 150994992 16777250 268435527\n")
        self.run_steps("take_launcher_keys")
        self.assertEqual(self.calls(), [])

    def test_restore_gives_the_plasma_launcher_its_keys_back(self):
        self.run_steps("take_launcher_keys")
        self.calls()
        result = self.run_steps("restore_launcher_keys")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.calls(), [
            [*KGLOBALACCEL, "unregister", "ss", "konveyor-kontrol-panel", "toggle"],
            [*KGLOBALACCEL, "setForeignShortcut", "asai", "4", *LAUNCHER, "3", "150994992", "16777250", "268435527"],
        ])
        self.assertFalse(self.state.exists())

    def test_restore_without_taking_only_drops_the_kontrol_panel_shortcut(self):
        self.assertEqual(self.run_steps("restore_launcher_keys").returncode, 0)
        self.assertEqual(self.calls(), [[*KGLOBALACCEL, "unregister", "ss", "konveyor-kontrol-panel", "toggle"]])

    def test_installs_the_service_the_bus_activation_and_the_application(self):
        result = self.run_steps("install_kontrol_panel_service")
        self.assertEqual(result.returncode, 0, result.stderr)
        panel = self.widgets / "portals" / "kontrol-panel"
        command = f"{self.stubs / 'konveyor-kontrol-panel'} {panel}"
        unit = (self.home / ".config" / "systemd" / "user" / "konveyor-kontrol-panel.service").read_text()
        self.assertIn(f"ExecStart={command}\n", unit)
        self.assertIn("Type=dbus\nBusName=org.devl0rd.KontrolPanel\n", unit)
        self.assertIn("WantedBy=graphical-session.target", unit)
        activation = (self.home / ".local" / "share" / "dbus-1" / "services" / "org.devl0rd.KontrolPanel.service").read_text()
        self.assertIn(f"Exec={command}\n", activation)
        self.assertIn("SystemdService=konveyor-kontrol-panel.service", activation)
        desktop = (self.home / ".local" / "share" / "applications" / "org.devl0rd.KontrolPanel.desktop").read_text()
        self.assertIn(f"Exec={self.home}/.local/bin/portal-launcher toggle\n", desktop)
        for staged in ("Main.qml", "Overlay.qml", "ConfigWindow.qml", "config/main.xml", "LauncherView.qml", "pages/HomePage.qml", "lib/FileWatcher.qml", "lib/GameArt.qml"):
            self.assertTrue((panel / staged).is_file(), staged)
        calls = self.calls()
        self.assertIn(["systemctl", "--user", "enable", "konveyor-kontrol-panel.service"], calls)
        self.assertIn(["systemctl", "--user", "restart", "konveyor-kontrol-panel.service"], calls)

    def test_install_fails_visibly_without_the_kontrol_panel_program(self):
        (self.stubs / "konveyor-kontrol-panel").unlink()
        result = self.run_steps("install_kontrol_panel_service")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("konveyor-kontrol-panel is missing", result.stderr)

    def test_remove_undoes_the_install(self):
        self.run_steps("install_kontrol_panel_service")
        result = self.run_steps("remove_kontrol_panel_service")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.home / ".config" / "systemd" / "user" / "konveyor-kontrol-panel.service").exists())
        self.assertFalse((self.home / ".local" / "share" / "dbus-1").exists())
        self.assertFalse((self.home / ".local" / "share" / "applications" / "org.devl0rd.KontrolPanel.desktop").exists())
        self.assertIn(["systemctl", "--user", "disable", "--now", "konveyor-kontrol-panel.service"], self.calls())


if __name__ == "__main__":
    unittest.main()
