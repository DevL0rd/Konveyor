#!/usr/bin/env python3
import importlib.machinery
import importlib.util
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
SCRIPT = REPO / "src" / "cheatsheet" / "konveyor-cheatsheet.in"


def load_module():
    loader = importlib.machinery.SourceFileLoader("konveyor_cheatsheet", str(SCRIPT))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    return module


class TestCheatsheetEntries(unittest.TestCase):
    def setUp(self):
        self.module = load_module()
        self.temporary = tempfile.TemporaryDirectory()
        self.module.KGLOBALSHORTCUTS = Path(self.temporary.name) / "kglobalshortcutsrc"

    def tearDown(self):
        self.temporary.cleanup()

    def entries(self, text):
        self.module.KGLOBALSHORTCUTS.write_text(text)
        return [entry for section, entry in self.module.kde_entries(set()) if section == self.module.WIDGETS_SECTION]

    def test_lists_the_kontrol_panel_keys(self):
        entries = self.entries("[konveyor-kontrol-panel]\n_k_friendly_name=Kontrol Panel\n"
                               "toggle=Meta\\tAlt+F1,Meta\\tAlt+F1,Open or close the Kontrol Panel\n")
        self.assertIn({"action": "Kontrol Panel: open", "keys": ["Meta", "Alt+F1"]}, entries)

    def test_follows_keys_the_user_changed(self):
        entries = self.entries("[konveyor-kontrol-panel]\ntoggle=Meta+Space,Meta\\tAlt+F1,Open or close the Kontrol Panel\n")
        self.assertIn({"action": "Kontrol Panel: open", "keys": ["Meta+Space"]}, entries)

    def test_leaves_out_a_kontrol_panel_without_keys(self):
        entries = self.entries("[konveyor-kontrol-panel]\ntoggle=none,Meta\\tAlt+F1,Open or close the Kontrol Panel\n")
        self.assertEqual([entry for entry in entries if entry["action"] == "Kontrol Panel: open"], [])


if __name__ == "__main__":
    unittest.main()
