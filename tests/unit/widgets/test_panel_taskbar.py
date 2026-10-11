#!/usr/bin/env python3
import subprocess
import sys
import unittest
from pathlib import Path

from appletsrc_harness import SERVICE, AppletsrcCase


FIXTURE = """[Containments][1]
plugin=org.kde.panel

[Containments][1][Applets][2]
plugin=org.devl0rd.portal.launcher
konveyorReplaced=org.kde.plasma.kickoff

[Containments][1][Applets][3]
plugin=org.kde.plasma.pager

[Containments][1][Applets][4]
immutability=1
plugin=org.kde.plasma.icontasks

[Containments][1][Applets][4][Configuration][General]
launchers=applications:org.kde.dolphin.desktop,applications:firefox.desktop
groupingStrategy=0

[Containments][1][Applets][5]
plugin=org.kde.plasma.systemtray

[Containments][1][General]
AppletOrder=2;3;4;5

[Containments][6]
plugin=org.kde.desktopcontainment
"""


class TestPanelTaskbar(AppletsrcCase):
    script = SERVICE / "panel-taskbar"
    fixture = FIXTURE

    def run_script(self, *arguments):
        return subprocess.run([sys.executable, str(self.script), *arguments, str(self.path)], capture_output=True, text=True, check=True)

    def applet(self, text, applet, panel=1):
        return text.split(f"[Containments][{panel}][Applets][{applet}]\n", 1)[1].split("\n[", 1)[0].rstrip("\n") + "\n"

    def test_install_swaps_the_task_manager_and_keeps_its_pins(self):
        self.assertEqual(self.run_script("install").stdout, "Konveyor Taskbar set up next to the Kontrol Panel\n")
        text = self.path.read_text()
        self.assertEqual(self.applet(text, 4), "immutability=1\nplugin=org.devl0rd.taskbar\nkonveyorReplaced=org.kde.plasma.icontasks\n"
                         "konveyorOriginalOrder=2;3;4;5\nkonveyorInstalledOrder=2;4;3;5\n")
        self.assertIn("launchers=applications:org.kde.dolphin.desktop,applications:firefox.desktop", text)
        self.assertIn("AppletOrder=2;4;3;5", text)
        self.assertNotIn("plugin=org.kde.plasma.icontasks", text)

    def test_install_is_idempotent(self):
        self.run_script("install")
        before = self.path.read_text()
        self.assertEqual(self.run_script("install").stdout, "")
        self.assertEqual(self.path.read_text(), before)

    def test_install_adds_the_taskbar_after_the_launcher_without_a_task_manager(self):
        self.path.write_text(FIXTURE.replace("org.kde.plasma.icontasks", "org.kde.plasma.digitalclock"))
        self.run_script("install")
        text = self.path.read_text()
        self.assertEqual(self.applet(text, 7), "immutability=1\nplugin=org.devl0rd.taskbar\nkonveyorAdded=true\n")
        self.assertIn("AppletOrder=2;7;3;4;5", text)
        self.assertIn("plugin=org.kde.plasma.digitalclock", text)

    def test_install_without_a_launcher_or_order_puts_the_taskbar_first(self):
        self.path.write_text("[Containments][3]\nplugin=org.kde.panel\n\n[Containments][3][Applets][4]\nplugin=org.kde.plasma.pager\n")
        self.run_script("install")
        self.assertIn("[Containments][3][General]\nAppletOrder=5;4\n", self.path.read_text())

    def test_every_task_manager_goes_and_their_pins_are_merged(self):
        self.path.write_text(FIXTURE.replace("AppletOrder=2;3;4;5", "AppletOrder=2;3;8;4;5")
                             + "\n[Containments][1][Applets][8]\nplugin=org.kde.plasma.taskmanager\n\n"
                             "[Containments][1][Applets][8][Configuration][General]\nlaunchers=applications:kate.desktop,applications:firefox.desktop\n")
        self.run_script("install")
        text = self.path.read_text()
        self.assertIn("plugin=org.devl0rd.taskbar\nkonveyorReplaced=org.kde.plasma.icontasks", self.applet(text, 4))
        self.assertIn("launchers=applications:org.kde.dolphin.desktop,applications:firefox.desktop,applications:kate.desktop", text)
        self.assertNotIn("[Containments][1][Applets][8]", text)
        self.assertIn("AppletOrder=2;4;3;5", text)

    def test_pins_of_a_removed_task_manager_reach_a_taskbar_without_settings(self):
        self.path.write_text(FIXTURE.replace("[Containments][1][Applets][4]\nimmutability", "[Containments][1][Applets][7]\nplugin=org.kde.plasma.taskmanager\n\n"
                                             "[Containments][1][Applets][4]\nimmutability"))
        self.run_script("install")
        text = self.path.read_text()
        self.assertIn("[Containments][1][Applets][7][Configuration][General]\n"
                      "launchers=applications:org.kde.dolphin.desktop,applications:firefox.desktop\n", text)
        self.assertNotIn("[Containments][1][Applets][4]", text)

    def test_the_taskbar_joins_the_launcher_panel_only(self):
        self.path.write_text(FIXTURE + "\n[Containments][10]\nplugin=org.kde.panel\n\n[Containments][10][Applets][11]\n"
                             "plugin=org.kde.plasma.taskmanager\n")
        self.path.write_text(self.path.read_text().replace("plugin=org.kde.plasma.icontasks", "plugin=org.kde.plasma.pager"))
        self.run_script("install")
        text = self.path.read_text()
        self.assertIn("[Containments][1][Applets][12]\nimmutability=1\nplugin=org.devl0rd.taskbar", text)
        self.assertIn("[Containments][10][Applets][11]\nplugin=org.kde.plasma.taskmanager", text)

    def round_trip_with_launcher(self, before):
        self.path.write_text(before)
        launcher = [sys.executable, str(SERVICE / "panel-launcher")]
        for command in ("install", "uninstall"):
            subprocess.run([*launcher, command, str(self.path)], capture_output=True, text=True, check=True)
            self.run_script(command)
        self.assertEqual(self.path.read_text(), before)

    def test_uninstall_restores_the_default_plasma_order_reported_by_ci(self):
        plugins = {3: "org.kde.plasma.kickoff", 4: "org.kde.plasma.pager", 5: "org.kde.plasma.icontasks",
                   6: "org.kde.plasma.systemtray", 7: "org.kde.plasma.digitalclock", 14: "org.kde.plasma.marginsseparator",
                   15: "org.kde.plasma.showdesktop"}
        before = "[Containments][1]\nplugin=org.kde.panel\n\n"
        before += "".join(f"[Containments][1][Applets][{applet}]\nplugin={plugin}\n\n"
                          for applet, plugin in plugins.items())
        before += "[Containments][1][General]\nAppletOrder=3;4;5;6;7;14;15\n"
        self.round_trip_with_launcher(before)

    def test_uninstall_restores_the_original_panel_order(self):
        self.run_script("install")
        self.run_script("uninstall")
        self.assertEqual(self.path.read_text(), FIXTURE)

    def test_uninstall_restores_position_and_keeps_unrelated_panel_edits(self):
        self.run_script("install")
        self.path.write_text(self.path.read_text().replace("AppletOrder=2;4;3;5", "AppletOrder=2;4;8;5;3")
                             + "\n[Containments][1][Applets][8]\nplugin=org.kde.plasma.digitalclock\n")
        self.run_script("uninstall")
        text = self.path.read_text()
        self.assertIn("AppletOrder=2;8;5;3;4", text)
        self.assertIn("[Containments][1][Applets][8]\nplugin=org.kde.plasma.digitalclock", text)
        self.assertNotIn("konveyor", self.applet(text, 4))

    def test_uninstall_keeps_a_taskbar_position_changed_by_the_user(self):
        self.run_script("install")
        self.path.write_text(self.path.read_text().replace("AppletOrder=2;4;3;5", "AppletOrder=2;3;5;4"))
        self.run_script("uninstall")
        self.assertIn("AppletOrder=2;3;5;4", self.path.read_text())

    def test_uninstall_uses_a_surviving_original_neighbor(self):
        self.run_script("install")
        self.path.write_text(self.path.read_text().replace("AppletOrder=2;4;3;5", "AppletOrder=2;4;5")
                             .replace("[Containments][1][Applets][3]\nplugin=org.kde.plasma.pager\n\n", ""))
        self.run_script("uninstall")
        self.assertIn("AppletOrder=2;4;5", self.path.read_text())

    def test_uninstall_launcher_first_restores_the_complete_original_config(self):
        for plugin in ("org.kde.plasma.kickoff", "org.kde.plasma.digitalclock"):
            with self.subTest(plugin=plugin):
                before = FIXTURE.replace("org.devl0rd.portal.launcher", plugin).replace("konveyorReplaced=org.kde.plasma.kickoff\n", "")
                self.round_trip_with_launcher(before)

    def test_uninstall_restores_a_task_manager_that_originally_came_first(self):
        before = FIXTURE.replace("AppletOrder=2;3;4;5", "AppletOrder=4;2;3;5")
        self.path.write_text(before)
        self.run_script("install")
        self.run_script("uninstall")
        self.assertEqual(self.path.read_text(), before)

    def test_uninstall_keeps_the_order_when_the_user_removed_the_taskbar_from_it(self):
        self.run_script("install")
        self.path.write_text(self.path.read_text().replace("AppletOrder=2;4;3;5", "AppletOrder=2;3;5"))
        self.run_script("uninstall")
        self.assertIn("AppletOrder=2;3;5", self.path.read_text())
        self.assertNotIn("konveyor", self.applet(self.path.read_text(), 4))

    def test_uninstall_keeps_a_legacy_taskbar_position_without_order_metadata(self):
        self.path.write_text(FIXTURE.replace("plugin=org.kde.plasma.icontasks", "plugin=org.devl0rd.taskbar\nkonveyorReplaced=org.kde.plasma.icontasks"))
        self.run_script("uninstall")
        self.assertEqual(self.path.read_text(), FIXTURE)

    def test_uninstall_gives_the_task_manager_back_after_the_launcher(self):
        self.path.write_text(FIXTURE.replace("AppletOrder=2;3;4;5", "AppletOrder=2;4;3;5"))
        before = self.path.read_text()
        self.run_script("install")
        self.assertEqual(self.run_script("uninstall").stdout, "Restored Plasma's task manager\n")
        self.assertEqual(self.path.read_text(), before)

    def test_uninstall_restores_the_task_manager_with_the_pins_of_the_taskbar(self):
        self.run_script("install")
        self.path.write_text(self.path.read_text().replace("applications:firefox.desktop", "applications:org.kde.konsole.desktop"))
        self.run_script("uninstall")
        text = self.path.read_text()
        self.assertEqual(self.applet(text, 4), "immutability=1\nplugin=org.kde.plasma.icontasks\n")
        self.assertIn("launchers=applications:org.kde.dolphin.desktop,applications:org.kde.konsole.desktop", text)
        self.assertIn("AppletOrder=2;3;4;5", text)

    def test_uninstall_removes_an_added_taskbar(self):
        self.path.write_text(FIXTURE.replace("org.kde.plasma.icontasks", "org.kde.plasma.digitalclock"))
        before = self.path.read_text()
        self.run_script("install")
        self.run_script("uninstall")
        self.assertEqual(self.path.read_text(), before)

    def test_uninstall_without_a_taskbar_changes_nothing(self):
        self.assertEqual(self.run_script("uninstall").stdout, "")
        self.assertEqual(self.path.read_text(), FIXTURE)

    def taskbarrc(self):
        return Path(self.temporary.name) / "konveyor" / "taskbarrc"

    def test_settings_move_into_taskbarrc_once(self):
        self.run_script("install")
        self.path.write_text(self.path.read_text().replace("groupingStrategy=0\n", "groupingStrategy=0\ngroupMode=2\nshowApps=false\n"))
        self.taskbarrc().parent.mkdir()
        self.taskbarrc().write_text("[General]\niconSpacing=6\nshowApps=true\n")
        result = self.main("settings", str(self.path), str(self.taskbarrc()))
        self.assertEqual(result.stdout, "Konveyor Taskbar settings moved to Konveyor Settings\n")
        stored = self.taskbarrc().read_text()
        self.assertIn("iconSpacing=6\nshowApps=true\n", stored)
        self.assertIn("launchers=applications:org.kde.dolphin.desktop,applications:firefox.desktop\n", stored)
        self.assertIn("groupMode=2\n", stored)
        self.assertIn("groupingStrategy=0\n", stored)
        self.assertNotIn("[Containments][1][Applets][4][Configuration][General]", self.path.read_text())
        before = (self.path.read_text(), stored)
        self.assertEqual(self.main("settings", str(self.path), str(self.taskbarrc())).stdout, "")
        self.assertEqual((self.path.read_text(), self.taskbarrc().read_text()), before)

    def test_settings_create_taskbarrc(self):
        self.run_script("install")
        self.main("settings", str(self.path), str(self.taskbarrc()))
        self.assertEqual(self.taskbarrc().read_text(),
                         "[General]\nlaunchers=applications:org.kde.dolphin.desktop,applications:firefox.desktop\ngroupingStrategy=0\n\n")

    def test_uninstall_gives_the_task_manager_the_pins_from_taskbarrc(self):
        self.run_script("install")
        self.main("settings", str(self.path), str(self.taskbarrc()))
        self.taskbarrc().write_text("[General]\nlaunchers=applications:org.kde.konsole.desktop\n")
        self.assertEqual(self.main("uninstall", str(self.path), str(self.taskbarrc())).stdout, "Restored Plasma's task manager\n")
        text = self.path.read_text()
        self.assertEqual(self.applet(text, 4), "immutability=1\nplugin=org.kde.plasma.icontasks\n")
        self.assertIn("[Containments][1][Applets][4][Configuration][General]\nlaunchers=applications:org.kde.konsole.desktop\n", text)

    def test_install_fails_without_a_panel(self):
        self.path.write_text("[Containments][6]\nplugin=org.kde.desktopcontainment\n")
        result = self.main("install", str(self.path), code=1)
        self.assertIn("no Plasma panel", result.stderr)

    def test_usage_and_missing_files(self):
        for arguments in ((), ("install",), ("install", "a", "b"), ("settings", "a"), ("uninstall", "a", "b", "c"), ("move", str(self.path))):
            self.assertIn("usage: panel-taskbar install <appletsrc>", self.main(*arguments, code=2).stderr)
        missing = Path(self.temporary.name) / "missing"
        for command in ("install", "uninstall"):
            self.assertEqual(self.main(command, str(missing)).stdout, "")
        self.assertFalse(missing.exists())


if __name__ == "__main__":
    unittest.main()
