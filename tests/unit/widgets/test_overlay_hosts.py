#!/usr/bin/env python3
import unittest
from pathlib import Path

from appletsrc_harness import SERVICE, AppletsrcCase


FIXTURE = """[Containments][1][Applets][2]
plugin=org.kde.plasma.systemtray

[Containments][1][Applets][2][General]
extraItems=org.kde.plasma.volume
hiddenItems=org.kde.plasma.clipboard
knownItems=org.kde.plasma.volume

[Containments][3][Applets][4]
plugin=org.kde.plasma.systemtray

[Containments][3][Applets][4][General]
extraItems=org.devl0rd.sysmon.overlay,org.kde.plasma.networkmanagement
hiddenItems=org.devl0rd.sysmon.overlay

[Containments][3][Applets][4][Applets][9]
plugin=org.devl0rd.sysmon.overlay

[Containments][3][Applets][4][Applets][9][Configuration]
test=true
"""


class TestOverlayHosts(AppletsrcCase):
    script = SERVICE / "overlay-hosts"
    fixture = FIXTURE

    def configure(self, install):
        groups = self.module.read_groups(self.path)
        result, changed = self.module.configure(groups, install)
        self.module.write_groups(self.path, result)
        return changed, self.path.read_text()

    def test_install_adds_hidden_hosts_to_only_first_tray(self):
        changed, text = self.configure(True)
        self.assertTrue(changed)
        first = text.split("[Containments][3][Applets][4]", 1)[0]
        for host in self.module.HOSTS:
            self.assertIn(host, first)
        second = text.split("[Containments][3][Applets][4]", 1)[1]
        for host in self.module.HOSTS:
            self.assertNotIn(host, second)
        self.assertNotIn("[Applets][9]", text)

    def test_install_is_idempotent(self):
        self.configure(True)
        changed, _ = self.configure(True)
        self.assertFalse(changed)

    def test_uninstall_removes_hosts_and_child_groups(self):
        self.configure(True)
        changed, text = self.configure(False)
        self.assertTrue(changed)
        for host in self.module.HOSTS:
            self.assertNotIn(host, text)

    def test_usage(self):
        usage = "usage: overlay-hosts install|uninstall <plasma-org.kde.plasma.desktop-appletsrc>\n"
        for arguments in ((), ("install",), ("remove", str(self.path)), ("install", str(self.path), "extra")):
            self.assertEqual(self.main(*arguments, code=2).stderr, usage)

    def test_a_missing_file_is_left_alone(self):
        missing = Path(self.temporary.name) / "missing"
        for mode in ("install", "uninstall"):
            self.assertEqual(self.main(mode, str(missing)).stdout, "")
        self.assertFalse(missing.exists())

    def test_install_needs_a_tray(self):
        self.path.write_text("[Containments][1]\nplugin=org.kde.panel\n")
        result = self.main("install", str(self.path), code=1)
        self.assertEqual(result.stderr, "overlay-hosts: no Plasma system tray was found\n")
        self.assertEqual(self.main("uninstall", str(self.path)).stdout, "")
        self.assertEqual(self.path.read_text(), "[Containments][1]\nplugin=org.kde.panel\n")

    def test_main_reports_changes_only(self):
        self.assertEqual(self.main("install", str(self.path)).stdout, "Konveyor monitor overlay hosts installed\n")
        installed = self.path.read_text()
        self.assertEqual(self.main("install", str(self.path)).stdout, "")
        self.assertEqual(self.path.read_text(), installed)
        self.assertEqual(self.main("uninstall", str(self.path)).stdout, "Konveyor monitor overlay hosts removed\n")
        self.assertEqual(self.main("uninstall", str(self.path)).stdout, "")

    def test_install_keeps_other_items_in_place(self):
        _, text = self.configure(True)
        hosts = ",".join(self.module.HOSTS)
        self.assertIn("[Containments][1][Applets][2][General]\nextraItems=org.kde.plasma.volume," + hosts + "\nhiddenItems="
                      "org.kde.plasma.clipboard," + hosts + "\nknownItems=org.kde.plasma.volume," + hosts + "\n\n", text)
        self.assertIn("[Containments][3][Applets][4][General]\nextraItems=org.kde.plasma.networkmanagement\n\n", text)

    def test_a_tray_without_settings_gets_them(self):
        self.path.write_text("[Containments][1][Applets][2]\nplugin=org.kde.plasma.systemtray\n")
        _, text = self.configure(True)
        hosts = ",".join(self.module.HOSTS)
        self.assertEqual(text, "[Containments][1][Applets][2]\nplugin=org.kde.plasma.systemtray\n[Containments][1][Applets][2]"
                         f"[General]\nextraItems={hosts}\nhiddenItems={hosts}\nknownItems={hosts}\n")
        changed, text = self.configure(False)
        self.assertTrue(changed)
        self.assertEqual(text, "[Containments][1][Applets][2]\nplugin=org.kde.plasma.systemtray\n[Containments][1][Applets][2]"
                         "[General]\n")

    def test_only_the_host_applets_are_dropped(self):
        self.path.write_text(self.path.read_text() + "\n[Containments][3][Applets][4][Applets][91]\nplugin=org.kde.plasma.battery\n"
                             "\n[Containments][3][Applets][4][Applets][91][Configuration]\nkept=true\n")
        _, text = self.configure(False)
        self.assertNotIn("[Applets][9]\n", text)
        self.assertNotIn("test=true", text)
        self.assertIn("[Containments][3][Applets][4][Applets][91]\nplugin=org.kde.plasma.battery", text)
        self.assertIn("kept=true", text)

    def test_hosts_in_the_first_tray_are_kept_on_install(self):
        self.path.write_text(self.path.read_text().replace("[Containments][3][Applets][4][Applets][9]",
                                                           "[Containments][1][Applets][2][Applets][9]"))
        _, text = self.configure(True)
        self.assertIn("[Containments][1][Applets][2][Applets][9]\nplugin=org.devl0rd.sysmon.overlay", text)
        _, text = self.configure(False)
        self.assertNotIn("[Applets][9]", text)

    def test_uninstall_cleans_every_tray(self):
        self.configure(True)
        self.path.write_text(self.path.read_text().replace("extraItems=org.kde.plasma.networkmanagement",
                                                           "extraItems=org.devl0rd.procmon.overlay,org.kde.plasma.networkmanagement"))
        _, text = self.configure(False)
        self.assertIn("extraItems=org.kde.plasma.volume\n", text)
        self.assertIn("extraItems=org.kde.plasma.networkmanagement\n", text)
        self.assertNotIn("overlay", text)


if __name__ == "__main__":
    unittest.main()
