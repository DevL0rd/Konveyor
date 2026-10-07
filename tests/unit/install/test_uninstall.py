#!/usr/bin/env python3
import json
import shutil
import subprocess
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import HarnessTest

KEPT = ("home/.config/konveyor", "home/.config/Linux-System-Monitor", "home/.config/Linux-Process-Mon",
        "home/.config/Linux-Router-Monitor", "home/.config/Plasma-App-Portal")
APPLETS = """[Containments][1]
plugin=org.kde.panel

[Containments][1][Applets][2]
plugin=org.kde.plasma.kickoff

[Containments][1][Applets][3]
plugin=org.kde.plasma.systemtray

[Containments][1][Applets][4]
plugin=org.kde.plasma.icontasks

[Containments][1][Applets][4][Configuration][General]
launchers=applications:org.kde.dolphin.desktop

[Containments][1][Applets][3][General]
extraItems=org.kde.plasma.clipboard
hiddenItems=org.kde.plasma.clipboard
knownItems=org.kde.plasma.clipboard

[Containments][1][General]
AppletOrder=2;4;3

[Containments][5]
plugin=org.kde.desktopcontainment

[Containments][5][General]
ToolBoxButtonState=topcenter
"""
PLASMA = """
const state = JSON.parse(process.argv[2]);
const shell = kind => state[kind].map(item => ({
    ...item,
    currentConfigGroup: [],
    readConfig(key, fallback) { return (this.config[this.currentConfigGroup.join("/")] || {})[key] ?? fallback; },
    writeConfig(key, value) { (this.config[this.currentConfigGroup.join("/")] ??= {})[key] = value; },
}));
const shells = {panels: shell("panels"), desktops: shell("desktops")};
const panels = () => shells.panels;
const desktops = () => shells.desktops;
eval(process.argv[1]);
console.log(JSON.stringify({panels: shells.panels.map(({lengthMode, config}) => ({lengthMode, config})),
                            desktops: shells.desktops.map(({config}) => ({config}))}));
"""


def changes(before, after):
    kept = lambda path: path.startswith(KEPT)
    return {path: (before.get(path), after.get(path)) for path in before.keys() | after.keys()
            if before.get(path) != after.get(path) and not kept(path)}


class TestUninstall(HarnessTest):
    def setUp(self):
        super().setUp()
        self.harness.write(self.harness.home / ".config" / "plasma-org.kde.plasma.desktop-appletsrc", APPLETS)
        self.harness.set_stub_state("systemd", {"environment": {"QT_PLUGIN_PATH": "/opt/plugins"}, "enabled": []})
        self.plugins = self.harness.prefix / "lib" / "plugins" / "kwin" / "effects" / "plugins"

    def home_snapshot(self):
        return {path: value for path, value in self.harness.snapshot().items() if not path.startswith(("prefix", "system"))}

    def test_uninstall_leaves_the_home_and_the_session_as_they_were(self):
        before = self.home_snapshot()
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.assertSucceeded(self.harness.uninstall())
        self.assertEqual(changes(before, self.home_snapshot()), {})

    def test_install_puts_the_taskbar_after_the_kontrol_panel_with_the_old_pins(self):
        self.assertSucceeded(self.harness.install("--skip-deps"))
        config = self.harness.config("plasma-org.kde.plasma.desktop-appletsrc")
        self.assertEqual(config[("Containments", "1", "Applets", "4")]["plugin"], "org.devl0rd.taskbar")
        self.assertEqual(config[("Containments", "1", "Applets", "4", "Configuration", "General")]["launchers"], "applications:org.kde.dolphin.desktop")
        self.assertEqual(config[("Containments", "1", "General")]["AppletOrder"], "2;4;3")
        self.assertTrue((self.harness.home / ".local" / "state" / "konveyor" / "taskbar-set-up").exists())
        self.assertSucceeded(self.harness.uninstall())
        config = self.harness.config("plasma-org.kde.plasma.desktop-appletsrc")
        self.assertEqual(config[("Containments", "1", "Applets", "4")]["plugin"], "org.kde.plasma.icontasks")

    def test_uninstall_removes_every_file_in_the_manifest(self):
        manifest = (self.harness.prefix / "share" / "konveyor" / "install_manifest.txt").read_text().split("\n")
        self.assertSucceeded(self.harness.uninstall())
        calls = self.harness.calls("sudo")
        removed = [call[3] for call in calls if call[1:3] == ["rm", "-f"]]
        self.assertEqual([path for path in manifest if path not in removed], [])
        self.assertIn(["sudo", "rm", "-rf", str(self.harness.prefix / "share" / "konveyor")], calls)

    def test_uninstall_unloads_and_disables_every_konveyor_plugin(self):
        self.harness.write(self.harness.home / ".config" / "kwinrc",
                           "[Plugins]\nkonveyor_effect_1Enabled=true\nkonveyor_effect_0Enabled=false\nprocess_monitor_telemetry_1Enabled=true\n")
        self.harness.set_stub_state("kwin", {"running": True, "loaded": ["konveyor_effect_1", "process_monitor_telemetry_1"], "refuse": []})
        self.assertSucceeded(self.harness.uninstall())
        self.assertEqual(self.harness.config("kwinrc"), {})
        self.assertEqual(self.harness.stub_state("kwin")["loaded"], [])
        removed = [call[3] for call in self.harness.calls("sudo") if call[1:3] == ["rm", "-f"]]
        self.assertIn(str(self.plugins / "konveyor_effect_1.so"), removed)
        self.assertIn(str(self.plugins / "process_monitor_telemetry_1.so"), removed)

    def test_keep_widgets_leaves_the_widgets_and_their_state(self):
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.assertSucceeded(self.harness.uninstall("--keep-widgets"))
        state = self.harness.home / ".local" / "state" / "konveyor"
        self.assertTrue((state / "launcher-keys").exists())
        self.assertTrue((state / "games-keys").exists())
        self.assertFalse((state / "install-options").exists())
        self.assertTrue((self.harness.home / ".local" / "share" / "konveyor" / "widgets" / "uninstall.sh").exists())
        self.assertIn("konveyor-widgets.service", self.harness.stub_state("systemd")["enabled"])
        self.assertEqual(self.harness.config("kwinrc")[("Plugins",)]["process_monitor_telemetry_1700000000Enabled"], "true")
        self.assertNotIn("konveyor_effect_1700000000Enabled", self.harness.config("kwinrc")[("Plugins",)])

    def test_purge_deletes_the_config_and_the_default_keeps_it(self):
        config = self.harness.home / ".config" / "konveyor" / "config.kdl"
        self.harness.write(config, "layout {}\n")
        self.assertSucceeded(self.harness.uninstall("--keep-widgets"))
        self.assertTrue(config.exists())
        self.assertSucceeded(self.harness.uninstall("--keep-widgets", "--purge"))
        self.assertFalse(config.parent.exists())

    def test_uninstall_without_a_manifest_says_so(self):
        (self.harness.prefix / "share" / "konveyor" / "install_manifest.txt").unlink()
        result = self.assertSucceeded(self.harness.uninstall("--keep-widgets"))
        self.assertIn("No install manifest found", result.stdout)

    def test_options(self):
        self.assertIn("--keep-widgets", self.assertSucceeded(self.harness.uninstall("--help")).stdout)
        result = self.harness.uninstall("--bogus")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unknown option: --bogus", result.stderr)
        result = self.harness.uninstall(root=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("run uninstall.sh as your normal user", result.stderr)

    def plasma_script(self):
        scripts = self.harness.stub_state("plasma")["scripts"]
        self.assertEqual(len(scripts), 1)
        return scripts[0]

    def test_uninstall_gives_filled_panels_and_hidden_desktop_widgets_back(self):
        node = shutil.which("node")
        if node is None:
            self.skipTest("node runs the Plasma script")
        self.assertSucceeded(self.harness.uninstall("--keep-widgets"))
        shell = {"panels": [{"lengthMode": "fill", "config": {"Konveyor": {"savedLengthMode": "fit"}}},
                            {"lengthMode": "custom", "config": {}}],
                 "desktops": [{"config": {"General": {"hideDesktopWidgets": True}}}, {"config": {}}]}
        result = subprocess.run([node, "-e", PLASMA, self.plasma_script(), json.dumps(shell)], capture_output=True, text=True, check=True)
        self.assertEqual(json.loads(result.stdout), {
            "panels": [{"lengthMode": "fit", "config": {"Konveyor": {"savedLengthMode": ""}}}, {"lengthMode": "custom", "config": {}}],
            "desktops": [{"config": {"General": {"hideDesktopWidgets": False}}}, {"config": {"General": {"hideDesktopWidgets": False}}}],
        })

    def test_uninstall_says_when_plasma_could_not_restore_the_panels(self):
        self.harness.set_stub_state("plasma", {"running": False, "scripts": []})
        result = self.assertSucceeded(self.harness.uninstall("--keep-widgets"))
        self.assertIn("Plasma is not running", result.stdout)

    def test_the_widget_uninstaller_clears_the_leftover_desktop_and_panel_keys(self):
        applets = self.harness.home / ".config" / "plasma-org.kde.plasma.desktop-appletsrc"
        self.assertSucceeded(self.harness.install("--skip-deps"))
        applets.write_text(applets.read_text() + "\n[Containments][5][General]\nhideDesktopWidgets=true\n\n[Containments][1][Konveyor]\n"
                           "savedLengthMode=\n\n[Containments][2][Konveyor]\nsavedLengthMode=fit\n")
        self.assertSucceeded(self.harness.uninstall())
        config = self.harness.config("plasma-org.kde.plasma.desktop-appletsrc")
        self.assertNotIn("hideDesktopWidgets", config[("Containments", "5", "General")])
        self.assertNotIn(("Containments", "1", "Konveyor"), config)
        self.assertEqual(config[("Containments", "2", "Konveyor")], {"savedLengthMode": "fit"})


class TestAtomicUninstall(HarnessTest):
    system = "steamos"

    def test_uninstall_leaves_everything_as_it_was(self):
        self.harness.set_stub_state("systemd", {"environment": {"QML_IMPORT_PATH": "/opt/qml"}, "enabled": []})
        before = self.harness.snapshot()
        self.assertSucceeded(self.harness.install())
        self.assertEqual(self.harness.stub_state("podman")["containers"], {"konveyor-steamos": "docker.io/library/archlinux:latest"})
        self.assertSucceeded(self.harness.uninstall())
        self.assertEqual(changes(before, self.harness.snapshot()), {})
        self.assertEqual(self.harness.stub_state("podman"), {"containers": {}, "images": []})

if __name__ == "__main__":
    unittest.main()
