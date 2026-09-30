#!/usr/bin/env python3
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import HarnessTest

GAMES = ("org.devl0rd.portal.launcher.games.desktop", "_launch")
LAUNCHER = ("plasmashell", "activate application launcher")
PANEL = ("konveyor-kontrol-panel", "toggle")
GRID = ("kwin", "Grid View")


class TestShortcutTakeovers(HarnessTest):
    def setUp(self):
        super().setUp()

    def widgets(self, script="install.sh", *arguments):
        return self.assertSucceeded(self.harness.run(str(self.harness.source / "widgets" / script), *arguments))

    def test_install_moves_meta_g_and_the_launcher_keys_to_the_kontrol_panel(self):
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.assertEqual(self.harness.shortcut(*GAMES), ["Meta+G"])
        self.assertEqual(self.harness.shortcut(*GRID), [])
        self.assertEqual(self.harness.shortcut(*PANEL), ["Meta", "Alt+F1"])
        self.assertEqual(self.harness.shortcut(*LAUNCHER), [])

    def test_uninstall_gives_every_key_back(self):
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.assertSucceeded(self.harness.uninstall())
        self.assertEqual(self.harness.shortcut(*GRID), ["Meta+G"])
        self.assertEqual(self.harness.shortcut(*LAUNCHER), ["Meta", "Alt+F1"])
        self.assertIsNone(self.harness.shortcut(*GAMES))
        self.assertIsNone(self.harness.shortcut(*PANEL))

    def test_grid_view_keeps_its_other_keys(self):
        self.harness.set_shortcut(*GRID, ["Meta+G", "Ctrl+F8"], "KWin", "Toggle Grid View")
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.assertEqual(self.harness.shortcut(*GRID), ["Ctrl+F8"])
        self.assertSucceeded(self.harness.uninstall())
        self.assertEqual(self.harness.shortcut(*GRID), ["Meta+G", "Ctrl+F8"])

    def test_an_update_keeps_the_keys_the_user_chose(self):
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.harness.set_shortcut(*GAMES, ["Meta+J"], *GAMES)
        self.harness.set_shortcut(*GRID, ["Meta+G"], "KWin", "Toggle Grid View")
        self.harness.set_shortcut(*LAUNCHER, ["Alt+F1"])
        self.harness.set_shortcut(*PANEL, ["Meta"])
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.widgets("install.sh", "--no-restart")
        self.assertEqual(self.harness.shortcut(*GAMES), ["Meta+J"])
        self.assertEqual(self.harness.shortcut(*GRID), ["Meta+G"])
        self.assertEqual(self.harness.shortcut(*LAUNCHER), ["Alt+F1"])
        self.assertEqual(self.harness.shortcut(*PANEL), ["Meta"])

    def test_a_second_install_makes_no_shortcut_changes(self):
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.harness.calls()
        self.assertSucceeded(self.harness.install("--skip-deps"))
        changes = [call for call in self.harness.calls("busctl") if call[6] in ("setShortcut", "setForeignShortcut", "unregister")]
        self.assertEqual(changes, [])

    def test_keys_the_effect_took_from_the_widgets_end_up_where_they_began(self):
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.harness.effect_takes("Meta+G", "Alt+F1")
        self.assertEqual(self.harness.shortcut(*GAMES), [])
        self.assertEqual(self.harness.shortcut(*PANEL), ["Meta"])
        self.assertSucceeded(self.harness.uninstall())
        self.assertEqual(self.harness.shortcut(*GRID), ["Meta+G"])
        self.assertEqual(self.harness.shortcut(*LAUNCHER), ["Meta", "Alt+F1"])

    def test_keys_the_effect_took_before_the_widgets_are_given_back_after_them(self):
        self.harness.effect_takes("Meta+G")
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.assertEqual(self.harness.shortcut(*GRID), [])
        self.assertSucceeded(self.harness.uninstall())
        self.assertEqual(self.harness.shortcut(*GRID), ["Meta+G"])
        order = [call[1] if call[0] == "konveyor" else call[6] for call in self.harness.calls() if call[0] in ("konveyor", "busctl")]
        self.assertLess(order.index("unregister"), order.index("restore-shortcuts"))

    def test_keep_widgets_leaves_the_widget_keys_until_the_widgets_go(self):
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.assertSucceeded(self.harness.uninstall("--keep-widgets"))
        self.assertEqual(self.harness.shortcut(*GAMES), ["Meta+G"])
        self.assertEqual(self.harness.shortcut(*PANEL), ["Meta", "Alt+F1"])
        self.widgets("uninstall.sh")
        self.assertEqual(self.harness.shortcut(*GRID), ["Meta+G"])
        self.assertEqual(self.harness.shortcut(*LAUNCHER), ["Meta", "Alt+F1"])

    def test_widgets_alone_install_and_uninstall_cleanly(self):
        self.widgets("install.sh", "--no-restart")
        self.assertEqual(self.harness.shortcut(*GAMES), ["Meta+G"])
        self.widgets("install.sh", "--no-restart")
        self.widgets("uninstall.sh")
        self.assertEqual(self.harness.shortcut(*GRID), ["Meta+G"])
        self.assertEqual(self.harness.shortcut(*LAUNCHER), ["Meta", "Alt+F1"])
        self.assertFalse((self.harness.home / ".local" / "state" / "konveyor" / "games-keys").exists())


if __name__ == "__main__":
    unittest.main()
