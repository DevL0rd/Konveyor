#!/usr/bin/env python3
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import HarnessTest

PLASMA = "plasma-plasmashell.service"


class TestPartialInstalls(HarnessTest):
    seeded = False
    def widgets(self, *arguments):
        return self.harness.run(str(self.harness.source / "widgets" / "install.sh"), *arguments)

    def plasma_calls(self):
        return [call[2] for call in self.harness.calls("systemctl") if call[-1] == PLASMA and call[2] in ("stop", "start")]

    def test_plasma_is_started_again_when_the_widget_setup_fails_while_it_is_stopped(self):
        self.harness.write(self.harness.home / ".config" / "plasma-org.kde.plasma.desktop-appletsrc",
                           "[Containments][5]\nplugin=org.kde.desktopcontainment\n")
        result = self.widgets()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("no Plasma system tray was found", result.stderr)
        self.assertEqual(self.plasma_calls(), ["stop", "start"])

    def test_plasma_is_started_again_when_the_widget_removal_fails_while_it_is_stopped(self):
        self.harness.home.joinpath(".config", "plasma-org.kde.plasma.desktop-appletsrc").write_bytes(b"[Containments][1]\nname=\xff\n")
        result = self.harness.run(str(self.harness.source / "widgets" / "uninstall.sh"))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("UnicodeDecodeError", result.stderr)
        self.assertEqual(self.plasma_calls(), ["stop", "start"])

    def test_an_uninstall_finishes_when_the_konveyor_program_is_already_gone(self):
        self.harness.seed_prefix()
        (self.harness.prefix / "bin" / "konveyor").unlink()
        self.harness.write(self.harness.home / ".local" / "state" / "konveyor" / "install-options", "widgets=true\n")
        result = self.assertSucceeded(self.harness.uninstall())
        self.assertIn("could not restore the KDE shortcuts", result.stdout)
        self.assertIn("Konveyor is uninstalled", result.stdout)
        self.assertFalse((self.harness.home / ".local" / "state" / "konveyor").exists())

    def test_an_uninstall_of_a_system_it_was_never_installed_on_succeeds(self):
        result = self.assertSucceeded(self.harness.uninstall())
        self.assertIn("nothing to remove", result.stdout)

    def test_widgets_from_before_the_merge_are_stopped_and_removed(self):
        home = self.harness.home
        units = home / ".config" / "systemd" / "user"
        legacy = ["linux-system-monitor.service", "linux-process-mon.service", "linux-router-monitor.service",
                  "linux-log-monitor.service", "portal-friends.service"]
        for unit in legacy:
            self.harness.write(units / unit, "[Service]\n")
        self.harness.set_stub_state("systemd", {"environment": {}, "enabled": legacy})
        leftovers = [units / "plasma-plasmashell.service.d" / "linux-log-monitor.conf",
                     home / ".config" / "environment.d" / "linux-router-monitor.conf",
                     home / ".config" / "plasma-workspace" / "env" / "linux-process-mon.sh",
                     home / ".local" / "bin" / "linux-plasma-keyboard-toggle",
                     home / ".local" / "state" / "linux-plasma-keyboard-toggle" / "state"]
        for path in leftovers:
            self.harness.write(path, "")
        self.assertSucceeded(self.widgets("--no-restart"))
        self.assertEqual(self.harness.stub_state("systemd")["enabled"], ["konveyor-kontrol-panel.service", "konveyor-widgets.service"])
        for path in [units / unit for unit in legacy] + leftovers:
            self.assertFalse(path.exists(), path)


if __name__ == "__main__":
    unittest.main()
