#!/usr/bin/env python3
import importlib.machinery
import importlib.util
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
SCRIPT = REPO / "widgets" / "service" / "overlay-hosts"


def load_module():
    loader = importlib.machinery.SourceFileLoader("overlay_hosts", str(SCRIPT))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    return module


class TestOverlayHosts(unittest.TestCase):
    def setUp(self):
        self.module = load_module()
        self.temporary = tempfile.TemporaryDirectory()
        self.path = Path(self.temporary.name) / "appletsrc"
        self.path.write_text("""[Containments][1][Applets][2]
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
""")

    def tearDown(self):
        self.temporary.cleanup()

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


if __name__ == "__main__":
    unittest.main()
