#!/usr/bin/env python3
import tempfile
import unittest
from pathlib import Path

from cheatsheet_module import load_module


GAMES = "[services][org.devl0rd.portal.launcher.games.desktop]\n"
KONTROL_PANEL = "[konveyor-kontrol-panel]\n_k_friendly_name=Kontrol Panel\n"


class TestCheatsheetEntries(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.module, _ = load_module(self.temporary.name)

    def tearDown(self):
        self.temporary.cleanup()

    def all_entries(self, text, taken=()):
        self.module.KGLOBALSHORTCUTS.write_text(text)
        return list(self.module.kde_entries(set(taken)))

    def entries(self, text):
        return [entry for section, entry in self.all_entries(text) if section == self.module.WIDGETS_SECTION]

    def test_reads_the_config_home_file(self):
        self.assertEqual(self.module.KGLOBALSHORTCUTS, Path(self.temporary.name) / "config" / "kglobalshortcutsrc")

    def test_lists_the_kontrol_panel_keys(self):
        entries = self.entries(KONTROL_PANEL + "toggle=Meta\\tAlt+F1,Meta\\tAlt+F1,Open or close the Kontrol Panel\n")
        self.assertIn({"action": "Kontrol Panel: open", "keys": ["Meta", "Alt+F1"]}, entries)

    def test_follows_keys_the_user_changed(self):
        entries = self.entries("[konveyor-kontrol-panel]\ntoggle=Meta+Space,Meta\\tAlt+F1,Open or close the Kontrol Panel\n")
        self.assertIn({"action": "Kontrol Panel: open", "keys": ["Meta+Space"]}, entries)

    def test_leaves_out_a_kontrol_panel_without_keys(self):
        entries = self.entries("[konveyor-kontrol-panel]\ntoggle=none,Meta\\tAlt+F1,Open or close the Kontrol Panel\n")
        self.assertEqual([entry for entry in entries if entry["action"] == "Kontrol Panel: open"], [])

    def test_games_key_leaves_out_the_default_and_description(self):
        entries = self.entries(GAMES + "_launch=Meta+G,Meta+G,Kontrol Panel: Games\n")
        self.assertEqual(entries, [{"action": "Kontrol Panel: open on Games", "keys": ["Meta+G"]}])

    def test_games_lists_every_key(self):
        entries = self.entries(GAMES + "_launch=Meta+G\\tCtrl+Alt+G,Meta+G,Kontrol Panel: Games\n")
        self.assertEqual(entries, [{"action": "Kontrol Panel: open on Games", "keys": ["Meta+G", "Ctrl+Alt+G"]}])

    def test_games_without_keys_is_left_out(self):
        self.assertEqual(self.entries(GAMES + "_launch=none,Meta+G,Kontrol Panel: Games\n"), [])
        self.assertEqual(self.entries(GAMES + "_launch=,Meta+G,Kontrol Panel: Games\n"), [])

    def test_splits_kde_shortcuts_with_several_keys(self):
        entries = self.all_entries("[kwin]\nShow Desktop=Meta+D\\tMeta+F12,Meta+F12,Peek at Desktop\n")
        self.assertEqual(entries, [("KDE Windows & Desktops", {"action": "Peek at Desktop", "keys": ["Meta+D", "Meta+F12"]})])

    def test_leaves_out_kde_keys_konveyor_took(self):
        entries = self.all_entries("[kwin]\nShow Desktop=Meta+D\\tMeta+F12,Meta+F12,Peek at Desktop\n", taken={"Meta+d"})
        self.assertEqual(entries, [("KDE Windows & Desktops", {"action": "Peek at Desktop", "keys": ["Meta+F12"]})])

    def test_leaves_out_a_kde_shortcut_whose_keys_are_all_taken(self):
        entries = self.all_entries("[kwin]\nShow Desktop=Meta+D\\tMeta+F12,Meta+F12,Peek at Desktop\n", taken={"Meta+d", "Meta+f12"})
        self.assertEqual(entries, [])

    def test_matches_taken_keys_in_any_spelling(self):
        entries = self.all_entries("[kwin]\nZoom=Meta++\\tShift+Meta+Up,none,Zoom In\n", taken={"Meta++", "Meta+Shift+up"})
        self.assertEqual(entries, [])

    def test_unescapes_commas_in_keys(self):
        entries = self.all_entries("[kwin]\nPrevious=Meta+\\,,Meta+\\,,Previous Desktop\n")
        self.assertEqual(entries, [("KDE Windows & Desktops", {"action": "Previous Desktop", "keys": ["Meta+,"]})])

    def test_keeps_commas_in_descriptions(self):
        entries = self.all_entries("[kwin]\nTile=Meta+T,none,Tile, then move\n")
        self.assertEqual(entries, [("KDE Windows & Desktops", {"action": "Tile, then move", "keys": ["Meta+T"]})])

    def test_names_each_kde_section(self):
        text = "".join(f"[{group}]\nsome action=Ctrl+{index},none,Action {index}\n" for index, group in enumerate(self.module.KDE_SECTIONS))
        sections = [section for section, _ in self.all_entries(text)]
        self.assertEqual(sections, list(self.module.KDE_SECTIONS.values()))

    def test_skips_unknown_groups_friendly_names_and_short_lines(self):
        text = ("[org.kde.konsole.desktop]\n_launch=Ctrl+Alt+T,none,Konsole\n"
                "[kwin]\n_k_friendly_name=KWin\nbroken line\nOld=Meta+O,none\nUnbound=none,none,Nothing\n")
        self.assertEqual(self.all_entries(text), [])

    def test_a_missing_file_gives_no_entries(self):
        self.assertEqual(list(self.module.kde_entries(set())), [])

    def test_shows_keys_given_back_after_a_takeover(self):
        taken = self.all_entries("[plasmashell]\nactivate application launcher=none,Meta\\tAlt+F1,Activate Application Launcher\n"
                                 "[kwin]\nExpose=none,Ctrl+F9,Toggle Present Windows (Current desktop)\n" + GAMES
                                 + "_launch=Meta+G,Meta+G,Kontrol Panel: Games\n" + KONTROL_PANEL
                                 + "toggle=Meta\\tAlt+F1,Meta\\tAlt+F1,Open or close the Kontrol Panel\n")
        self.assertEqual(taken, [(self.module.WIDGETS_SECTION, {"action": "Kontrol Panel: open on Games", "keys": ["Meta+G"]}),
                                 (self.module.WIDGETS_SECTION, {"action": "Kontrol Panel: open", "keys": ["Meta", "Alt+F1"]})])
        restored = self.all_entries("[plasmashell]\nactivate application launcher=Meta\\tAlt+F1,Meta\\tAlt+F1,Activate Application Launcher\n"
                                    "[kwin]\nExpose=Meta+G,Ctrl+F9,Toggle Present Windows (Current desktop)\n")
        self.assertEqual(restored, [("KDE Plasma", {"action": "Activate Application Launcher", "keys": ["Meta", "Alt+F1"]}),
                                    ("KDE Windows & Desktops", {"action": "Toggle Present Windows (Current desktop)", "keys": ["Meta+G"]})])

    def test_split_key_keeps_a_plus_key(self):
        self.assertEqual(self.module.split_key("Meta++"), ["Meta", "+"])
        self.assertEqual(self.module.split_key("Ctrl+Alt+T"), ["Ctrl", "Alt", "T"])
        self.assertEqual(self.module.split_key("+"), ["+"])

    def test_normalize_key_orders_modifiers_and_aliases_keys(self):
        self.assertEqual(self.module.normalize_key("Shift+Super+Page_Down"), "Meta+Shift+pgdown")
        self.assertEqual(self.module.normalize_key("Mod+Control+BracketLeft"), "Meta+Ctrl+[")
        self.assertEqual(self.module.normalize_key("Super+plus"), "Meta++")
        self.assertEqual(self.module.normalize_key("Win+Alt+Escape"), "Meta+Alt+esc")
        self.assertEqual(self.module.normalize_key("Hyper+A"), "Hyper+a")


if __name__ == "__main__":
    unittest.main()
