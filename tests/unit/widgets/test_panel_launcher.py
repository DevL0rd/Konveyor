#!/usr/bin/env python3
import importlib.machinery
import importlib.util
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

    def run_command(self, command, *arguments):
        groups = self.module.read_groups(self.path)
        if command == "open-page":
            result, changed = self.module.open_page(groups, *arguments)
        else:
            result, changed = getattr(self.module, command)(groups)
        self.module.write_groups(self.path, result)
        return changed, self.path.read_text()

    def general(self, text, applet):
        header = f"[Containments][1][Applets][{applet}][Configuration][General]"
        return text.split(header, 1)[1].split("\n[", 1)[0] if header in text else None

    def test_open_page_needs_a_panel_launcher(self):
        changed, text = self.run_command("open-page", "shortcuts")
        self.assertFalse(changed)
        self.assertNotIn("openPageOnStart", text)

    def test_open_page_marks_the_panel_launcher_that_replaced_the_menu(self):
        self.run_command("install")
        changed, text = self.run_command("open-page", "shortcuts")
        self.assertTrue(changed)
        self.assertIn("openPageOnStart=shortcuts", self.general(text, 2))
        self.assertEqual(text.count("openPageOnStart="), 1)
        self.assertNotIn("favoritesPortedToKAstats", text)

    def test_open_page_updates_existing_launcher_settings(self):
        self.run_command("install")
        self.path.write_text(self.path.read_text() + "\n[Containments][1][Applets][2][Configuration][General]\nicon=start-here\nopenPageOnStart=home\n")
        changed, text = self.run_command("open-page", "shortcuts")
        self.assertTrue(changed)
        general = self.general(text, 2)
        self.assertIn("icon=start-here", general)
        self.assertIn("openPageOnStart=shortcuts", general)
        self.assertNotIn("openPageOnStart=home", text)


if __name__ == "__main__":
    unittest.main()
