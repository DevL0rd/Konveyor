#!/usr/bin/env python3
import subprocess
import sys
import unittest
from pathlib import Path

from appletsrc_harness import SERVICE, AppletsrcCase


FIXTURE = """[Containments][1]
plugin=org.kde.panel

[Containments][1][Applets][2]
plugin=org.kde.plasma.kickoff

[Containments][1][Applets][2][Configuration][General]
favoritesPortedToKAstats=true

[Containments][1][Applets][3]
plugin=org.kde.plasma.systemtray

[Containments][5]
plugin=org.kde.desktopcontainment

[Containments][5][Applets][6]
plugin=org.devl0rd.portal.launcher
"""


class TestPanelLauncher(AppletsrcCase):
    script = SERVICE / "panel-launcher"
    fixture = FIXTURE

    def general(self, text, applet):
        header = f"[Containments][1][Applets][{applet}][Configuration][General]"
        return text.split(header, 1)[1].split("\n[", 1)[0] if header in text else None

    def migrate(self, target):
        return subprocess.run([sys.executable, str(self.script), "migrate", str(self.path), str(target)],
                              capture_output=True, text=True, check=True)

    def test_migrate_moves_the_launcher_settings_and_pins(self):
        self.run_script("install")
        self.path.write_text(self.path.read_text() + "\n[Containments][1][Applets][2][Configuration][General]\nicon=start-here\n"
                             "label=Start\nshowLabel=true\nopenPageOnStart=shortcuts\ncardWidth=70\ndefaultPage=apps\n"
                             "learnedRanking={\"a,b\":2}\n")
        target = Path(self.temporary.name) / "konveyor" / "kontrolpanelrc"
        result = self.migrate(target)
        self.assertIn("kontrolpanelrc", result.stdout)
        self.assertEqual(target.read_text(), "[General]\nfavoritesClient=org.kde.plasma.kicker.favorites.instance-2\n"
                         "cardWidth=70\ndefaultPage=apps\nlearnedRanking={\"a,b\":2}\n")

    def test_migrate_keeps_pins_of_a_launcher_without_settings(self):
        self.run_script("install")
        target = Path(self.temporary.name) / "kontrolpanelrc"
        self.migrate(target)
        self.assertEqual(target.read_text(), "[General]\nfavoritesClient=org.kde.plasma.kicker.favorites.instance-2\n")

    def test_migrate_never_overwrites_the_kontrol_panel_settings(self):
        self.run_script("install")
        target = Path(self.temporary.name) / "kontrolpanelrc"
        target.write_text("[General]\ncardWidth=40\n")
        result = self.migrate(target)
        self.assertEqual(result.stdout, "")
        self.assertEqual(target.read_text(), "[General]\ncardWidth=40\n")

    def test_migrate_takes_a_launcher_on_the_desktop(self):
        target = Path(self.temporary.name) / "kontrolpanelrc"
        self.migrate(target)
        self.assertEqual(target.read_text(), "[General]\nfavoritesClient=org.kde.plasma.kicker.favorites.instance-6\n")

    def test_migrate_without_a_launcher_leaves_the_defaults(self):
        self.path.write_text("[Containments][1]\nplugin=org.kde.panel\n\n[Containments][1][Applets][2]\nplugin=org.kde.plasma.kickoff\n")
        target = Path(self.temporary.name) / "kontrolpanelrc"
        result = self.migrate(target)
        self.assertEqual(result.stdout, "")
        self.assertFalse(target.exists())

    def run_script(self, *arguments):
        return subprocess.run([sys.executable, str(self.script), *arguments, str(self.path)], capture_output=True, text=True, check=True)

    def test_install_replaces_the_menu_with_the_launcher(self):
        self.run_script("install")
        text = self.path.read_text()
        self.assertIn("plugin=org.devl0rd.portal.launcher", text.split("[Containments][1][Applets][2]", 1)[1])
        self.assertIn("konveyorReplaced=org.kde.plasma.kickoff", text)
        self.assertNotIn("openPageOnStart", text)

    def test_install_leaves_an_existing_panel_launcher_alone(self):
        self.run_script("install")
        before = self.path.read_text()
        result = self.run_script("install")
        self.assertEqual(result.stdout, "")
        self.assertEqual(self.path.read_text(), before)

    def test_install_adds_the_launcher_first_without_a_menu_to_replace(self):
        self.path.write_text(self.path.read_text().replace("org.kde.plasma.kickoff", "org.kde.plasma.pager"))
        self.run_script("install")
        text = self.path.read_text()
        self.assertIn("plugin=org.devl0rd.portal.launcher\nkonveyorAdded=true", text.split("[Containments][1][Applets][7]", 1)[1])
        self.assertIn("AppletOrder=7;2;3", text)
        self.assertIn("plugin=org.kde.plasma.pager", text)

    def test_install_removes_every_other_menu(self):
        self.path.write_text(self.path.read_text() + "\n[Containments][1][Applets][4]\nplugin=org.kde.plasma.kicker\n\n"
                             "[Containments][1][Applets][4][Configuration]\nfavorites=a\n\n[Containments][1][General]\nAppletOrder=2;4;3\n")
        self.run_script("install")
        text = self.path.read_text()
        self.assertNotIn("org.kde.plasma.kicker", text)
        self.assertNotIn("[Containments][1][Applets][4]", text)
        self.assertIn("AppletOrder=2;3", text)
        self.assertIn("konveyorReplaced=org.kde.plasma.kickoff", text)

    def test_uninstall_removes_an_added_launcher(self):
        self.path.write_text(self.path.read_text().replace("org.kde.plasma.kickoff", "org.kde.plasma.pager")
                             + "\n[Containments][1][General]\nAppletOrder=2;3\n")
        before = self.path.read_text()
        self.run_script("install")
        self.run_script("uninstall")
        self.assertEqual(self.path.read_text(), before)

    def test_install_fails_without_a_panel(self):
        self.path.write_text("[Containments][5]\nplugin=org.kde.desktopcontainment\n")
        result = subprocess.run([sys.executable, str(self.script), "install", str(self.path)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn("no Plasma panel", result.stderr)

    def test_usage(self):
        for arguments in ((), ("install",), ("uninstall", "a", "b"), ("migrate", str(self.path)), ("move", str(self.path))):
            self.assertIn("usage: panel-launcher install|uninstall", self.main(*arguments, code=2).stderr)

    def test_a_missing_file_is_left_alone(self):
        missing = Path(self.temporary.name) / "missing"
        target = Path(self.temporary.name) / "kontrolpanelrc"
        for arguments in (("install", str(missing)), ("uninstall", str(missing)), ("migrate", str(missing), str(target))):
            self.assertEqual(self.main(*arguments).stdout, "")
        self.assertFalse(missing.exists() or target.exists())

    def test_uninstall_without_a_launcher_changes_nothing(self):
        before = self.path.read_text()
        self.assertEqual(self.run_script("uninstall").stdout, "")
        self.assertEqual(self.path.read_text(), before)

    def test_install_and_uninstall_report_what_they_did(self):
        self.assertEqual(self.run_script("install").stdout, "Kontrol Panel set up in the panel\n")
        self.assertEqual(self.run_script("uninstall").stdout, "Restored the original application launcher\n")

    def test_uninstall_restores_the_replaced_menu(self):
        self.run_script("install")
        self.path.write_text(self.path.read_text() + "\n[Containments][1][Applets][2][Configuration][General]\ncardWidth=70\n")
        self.run_script("uninstall")
        text = self.path.read_text()
        self.assertIn("[Containments][1][Applets][2]\nplugin=org.kde.plasma.kickoff\n", text)
        self.assertNotIn("konveyorReplaced", text)
        self.assertNotIn("[Containments][1][Applets][2][", text)
        self.assertIn("[Containments][5][Applets][6]\nplugin=org.devl0rd.portal.launcher", text)

    def test_install_next_to_an_existing_launcher_removes_the_menus(self):
        self.path.write_text(self.path.read_text() + "\n[Containments][1][Applets][8]\nplugin=org.devl0rd.portal.launcher\n"
                             "\n[Containments][1][General]\nAppletOrder=8;2;3\n")
        self.assertEqual(self.run_script("install").stdout, "Kontrol Panel set up in the panel\n")
        text = self.path.read_text()
        self.assertNotIn("kickoff", text)
        self.assertNotIn("[Containments][1][Applets][2]", text)
        self.assertIn("AppletOrder=8;3", text)
        self.assertEqual(self.run_script("uninstall").stdout, "")

    def test_menus_in_two_panels(self):
        self.path.write_text(self.path.read_text() + "\n[Containments][10]\nplugin=org.kde.panel\n\n[Containments][10][General]\n"
                             "AppletOrder=11;12\n\n[Containments][10][Applets][11]\nplugin=org.kde.plasma.kickerdash\n\n"
                             "[Containments][10][Applets][12]\nplugin=org.kde.plasma.digitalclock\n")
        self.run_script("install")
        text = self.path.read_text()
        self.assertIn("[Containments][1][Applets][2]\nplugin=org.devl0rd.portal.launcher", text)
        self.assertNotIn("kickerdash", text)
        self.assertIn("[Containments][10][General]\nAppletOrder=12\n", text)

    def test_the_launcher_joins_the_panel_with_the_task_manager(self):
        self.path.write_text("[Containments][1]\nplugin=org.kde.panel\n\n[Containments][1][Applets][2]\nplugin=org.kde.plasma.pager\n"
                             "\n[Containments][4]\nplugin=org.kde.panel\n\n[Containments][4][Applets][5]\n"
                             "plugin=org.kde.plasma.icontasks\n\n[Containments][4][General]\nlength=10\n")
        self.run_script("install")
        text = self.path.read_text()
        self.assertIn("[Containments][4][General]\nlength=10\nAppletOrder=6;5\n", text)
        self.assertIn("[Containments][4][Applets][6]\nimmutability=1\nplugin=org.devl0rd.portal.launcher\nkonveyorAdded=true\n", text)

    def test_the_launcher_joins_the_first_panel_without_a_task_manager(self):
        self.path.write_text("[Containments][7]\nplugin=org.kde.panel\n\n[Containments][3]\nplugin=org.kde.panel\n\n"
                             "[Containments][3][Applets][4]\nplugin=org.kde.plasma.pager\n")
        self.run_script("install")
        text = self.path.read_text()
        self.assertIn("[Containments][3][General]\nAppletOrder=8;4\n", text)
        self.assertIn("[Containments][3][Applets][8]\n", text)
        self.run_script("uninstall")
        self.assertNotIn("portal.launcher", self.path.read_text())
        self.assertIn("[Containments][3][General]\nAppletOrder=4\n", self.path.read_text())


if __name__ == "__main__":
    unittest.main()
