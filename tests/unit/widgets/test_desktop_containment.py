#!/usr/bin/env python3
import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from portal_harness import REPO, Sandbox


HIDER = REPO / "widgets" / "desktop-hider"
SCRIPT = HIDER / "desktop-containment"
PATCH = HIDER / "patches" / "desktopcontainment.patch"
CONTAINMENT = Path("plasma") / "plasmoids" / "org.kde.desktopcontainment"
INSTALLED = Path("/usr/share") / CONTAINMENT
TOOLS = ("bash", "cp", "dirname", "git", "mkdir", "mktemp", "mv", "readlink", "rm", "sed", "touch")
APPLETS = """[Containments][1][General]
hideDesktopWidgets=true

[Containments][2]
plugin=org.kde.panel

[Containments][7][General]
hideDesktopWidgets=false
"""


def unpatched_files():
    files, current = {}, None
    for line in PATCH.read_text().splitlines():
        if line.startswith("--- a/"):
            current = files.setdefault(line[len("--- a/"):], ["<!-- earlier lines -->"])
        elif current is not None and line[:1] in (" ", "-") and not line.startswith("---"):
            current.append(line[1:])
        elif current is not None and line == "":
            current.append("")
    return {name: "\n".join(lines) + "\n" for name, lines in files.items()}


class TestDesktopContainment(unittest.TestCase):
    def setUp(self):
        self.box = Sandbox(stubs=("kwriteconfig6",), tools=TOOLS)
        self.system = self.box.system_data / CONTAINMENT
        self.target = self.box.data / CONTAINMENT
        self.plasma("6.4")

    def tearDown(self):
        self.box.cleanup()

    def plasma(self, version, broken=False):
        shutil.rmtree(self.system, ignore_errors=True)
        for name, text in unpatched_files().items():
            if broken:
                text = text.replace("appletContainerComponent", "appletContainerDelegate")
            self.box.write(self.system / name, text)
        self.box.write(self.system / "metadata.json", '{"KPlugin": {"Version": "%s"}}\n' % version)

    def run_hider(self, *arguments):
        return subprocess.run([str(SCRIPT), *arguments], env=self.box.environment, capture_output=True, text=True)

    def install(self):
        result = self.run_hider("install")
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout

    def test_installs_a_patched_copy_of_the_containment(self):
        self.assertIn("Desktop widgets can now hide while windows cover the desktop", self.install())
        self.assertTrue((self.target / ".konveyor-managed").is_file())
        self.assertIn('<entry name="hideDesktopWidgets" type="Bool" hidden="true">', (self.target / "contents/config/main.xml").read_text())
        self.assertIn("opacity: Plasmoid.configuration.hideDesktopWidgets ? 0 : 1", (self.target / "contents/ui/main.qml").read_text())
        self.assertNotIn("hideDesktopWidgets", (self.system / "contents/ui/main.qml").read_text())

    def test_follows_a_plasma_upgrade(self):
        self.install()
        self.plasma("6.5")
        self.install()
        self.assertIn('"6.5"', (self.target / "metadata.json").read_text())
        self.assertIn("hideDesktopWidgets", (self.target / "contents/ui/main.qml").read_text())

    def test_steps_aside_when_an_upgrade_no_longer_takes_the_patch(self):
        self.install()
        self.plasma("7.0", broken=True)
        self.assertIn("doesn't take the patch", self.install())
        self.assertFalse(self.target.exists())

    def test_the_first_containment_on_the_data_dirs_wins(self):
        later = self.box.root / "later"
        shutil.copytree(self.system.parents[2], later)
        (later / CONTAINMENT / "metadata.json").write_text('{"KPlugin": {"Version": "old"}}\n')
        self.box.environment["XDG_DATA_DIRS"] = f"{self.box.root / 'empty'}::{self.box.system_data}:{later}"
        self.install()
        self.assertIn('"6.4"', (self.target / "metadata.json").read_text())

    def test_skips_when_plasma_has_no_desktop_containment(self):
        shutil.rmtree(self.system)
        self.assertIn(f"isn't in any of {self.box.system_data}", self.install())
        self.assertFalse(self.target.exists())

    def test_leaves_someone_elses_override_alone(self):
        self.box.write(self.target / "contents/ui/main.qml", "custom")
        self.assertIn("is someone else's override", self.install())
        self.assertEqual((self.target / "contents/ui/main.qml").read_text(), "custom")
        self.assertEqual(self.run_hider("uninstall").returncode, 0)
        self.assertEqual((self.target / "contents/ui/main.qml").read_text(), "custom")

    def test_takes_over_a_copy_from_the_old_widget_hider(self):
        self.box.write(self.target / ".linux-widget-hider-managed", "")
        self.install()
        self.assertTrue((self.target / ".konveyor-managed").is_file())
        self.assertFalse((self.target / ".linux-widget-hider-managed").exists())

    def test_removes_the_old_kwin_script_hider(self):
        script = self.box.data / "kwin" / "scripts" / "devl0rd-hide-desktop-widgets"
        self.box.write(script / ".linux-widget-hider-managed", "")
        dropins = [self.box.config / "systemd/user/plasma-plasmashell.service.d/linux-widget-hider.conf", self.box.config / "environment.d/linux-widget-hider.conf"]
        for path in dropins:
            self.box.write(path, "")
        self.install()
        self.assertFalse(script.exists())
        self.assertFalse(any(path.exists() for path in dropins))
        self.assertIn(["kwriteconfig6", "--file", "kwinrc", "--group", "Plugins", "--key", "devl0rd-hide-desktop-widgetsEnabled", "--delete"], self.box.calls())

    def test_keeps_a_kwin_script_it_did_not_install(self):
        script = self.box.data / "kwin" / "scripts" / "devl0rd-hide-desktop-widgets"
        self.box.write(script / "metadata.json", "{}")
        self.install()
        self.assertTrue(script.exists())

    def test_uninstall_removes_the_copy_and_the_setting(self):
        self.install()
        self.box.write(self.box.config / "plasma-org.kde.plasma.desktop-appletsrc", APPLETS)
        self.assertEqual(self.run_hider("uninstall").returncode, 0)
        self.assertFalse(self.target.exists())
        cleared = [call for call in self.box.calls() if "hideDesktopWidgets" in call]
        self.assertEqual([call[call.index("Containments") + 2] for call in cleared], ["1", "7"])
        self.assertTrue(all(call[-1] == "--delete" for call in cleared))

    def test_uninstall_without_anything_installed_succeeds(self):
        result = self.run_hider("uninstall")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual([call for call in self.box.calls() if "hideDesktopWidgets" in call], [])

    def test_unknown_commands_print_usage(self):
        for arguments in ([], ["reinstall"]):
            result = self.run_hider(*arguments)
            self.assertEqual(result.returncode, 2)
            self.assertIn("usage: desktop-containment install|uninstall", result.stderr)

    @unittest.skipUnless(INSTALLED.is_dir(), "Plasma's desktop containment is not installed here")
    def test_the_patch_applies_to_the_installed_plasma(self):
        with tempfile.TemporaryDirectory() as directory:
            copy = Path(directory) / "containment"
            shutil.copytree(INSTALLED, copy)
            result = subprocess.run(["git", "apply", "--check", "-p1", str(PATCH)], cwd=copy, capture_output=True, text=True,
                                    env={**os.environ, "GIT_CEILING_DIRECTORIES": directory})
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(re.search(r"appletContainerComponent", (copy / "contents/ui/main.qml").read_text()))


if __name__ == "__main__":
    unittest.main()
