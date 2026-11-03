#!/usr/bin/env python3
import unittest

from collectorharness import WIDGETS, CollectorTest

SCRIPT = WIDGETS / "process-monitor" / "bin" / "procmon_icons.py"


class TestProcmonIcons(CollectorTest):
    def setUp(self):
        super().setUp()
        self.applications = self.home / "applications"
        self.applications.mkdir()
        self.module = self.load(SCRIPT, "procmon_icons_direct")
        self.module.DESKTOP_DIRS = [str(self.applications)]

    def desktop(self, name, **fields):
        body = "".join("%s=%s\n" % item for item in fields.items())
        path = self.applications / name
        path.write_text("[Desktop Entry]\n" + body)
        return path

    def test_a_desktop_file_that_cannot_be_read_is_skipped(self):
        self.desktop("locked.desktop", Icon="locked-icon", Exec="locked").chmod(0)
        self.desktop("open.desktop", Icon="open-icon", Exec="open")
        icons, _ = self.module.build_icon_map()
        self.assertEqual(icons, {"open": "open-icon"})
        self.assertIsNone(self.module._parse_desktop(str(self.applications / "missing.desktop")))

    def test_try_exec_and_window_class_name_the_application_too(self):
        self.desktop("foo.desktop", Icon="foo-icon", Exec="foo-launcher %U", TryExec="/usr/bin/foo-real", StartupWMClass="FooWindow")
        icons, _ = self.module.build_icon_map()
        self.assertEqual({key: icons[key] for key in ("foo", "foo-launcher", "foo-real", "foowindow")}, {
            "foo": "foo-icon", "foo-launcher": "foo-icon", "foo-real": "foo-icon", "foowindow": "foo-icon"})

    def test_a_command_made_only_of_wrappers_and_options_has_no_name(self):
        self.assertEqual(self.module._exec_basename("env FOO=1 %U --flag"), "")

    def test_the_first_icon_lookup_builds_the_map(self):
        self.desktop("konsole.desktop", Icon="utilities-terminal", Exec="konsole")
        self.assertIsNone(self.module._icon_map)
        self.assertEqual(self.module.resolve_icon("Konsole"), "utilities-terminal")
        self.assertEqual(self.module.resolve_icon("nothing-known"), "")

    def test_a_flatpak_process_finds_its_icon_through_its_app_id(self):
        self.desktop("org.example.Paint.desktop", Icon="paint-icon", Exec="flatpak run org.example.Paint")
        self.fake.write("/proc/55/root/.flatpak-info", "[Instance]\nname=ignored\n[Application]\nname=org.example.Paint\n")
        self.assertIsNone(self.module._icon_map)
        self.assertEqual(self.module.resolve_process_icon(55, "100", "paint-wrapper"), "paint-icon")

    def test_a_process_identity_is_remembered(self):
        self.fake.write("/proc/70/environ", "A=1\0SteamAppId=440\0")
        self.module._steam_apps = {"440": ("Team Fortress", "tf2")}
        first = self.module.process_identity(70, "name", "icon")
        (self.home / "root/proc/70/environ").unlink()
        self.assertEqual((first, self.module.process_identity(70, "name", "icon")), (("Team Fortress", "tf2"),) * 2)

    def test_a_process_whose_environment_cannot_be_read_keeps_its_own_identity(self):
        self.assertEqual(self.module.process_identity(71, "name", "icon"), ("name", "icon"))


if __name__ == "__main__":
    unittest.main()
