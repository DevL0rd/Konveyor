#!/usr/bin/env python3
import importlib.machinery
import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
SCRIPT = REPO / "widgets" / "service" / "panel-launcher"


def load_module():
    loader = importlib.machinery.SourceFileLoader("panel_launcher", str(SCRIPT))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    return module


class TestPanelLauncher(unittest.TestCase):
    def setUp(self):
        self.module = load_module()
        self.temporary = tempfile.TemporaryDirectory()
        self.path = Path(self.temporary.name) / "appletsrc"
        self.path.write_text("""[Containments][1]
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
""")

    def tearDown(self):
        self.temporary.cleanup()

    def general(self, text, applet):
        header = f"[Containments][1][Applets][{applet}][Configuration][General]"
        return text.split(header, 1)[1].split("\n[", 1)[0] if header in text else None

    def migrate(self, target):
        return subprocess.run([sys.executable, str(SCRIPT), "migrate", str(self.path), str(target)],
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
        return subprocess.run([sys.executable, str(SCRIPT), *arguments, str(self.path)], capture_output=True, text=True, check=True)

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
        result = subprocess.run([sys.executable, str(SCRIPT), "install", str(self.path)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn("no Plasma panel", result.stderr)


if __name__ == "__main__":
    unittest.main()
