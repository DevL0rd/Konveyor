#!/usr/bin/env python3
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import MANAGERS, STUBS, TOOLS, HarnessTest

UNREACHABLE = ("docker", "ninja", "qdbus", "qdbus6", "dbus-send", "plasmashell", "kwin_wayland", "notify-send", "pkexec", "doas", "su",
               "chown", "loginctl", "flatpak", "rpm-ostree", "npm")


class TestHarness(HarnessTest):
    managers = MANAGERS

    def resolve(self, name):
        return self.harness.run("-c", 'command -v "$1" || true', "resolve", name).stdout.strip()

    def test_the_path_holds_only_stubs_and_basic_tools(self):
        self.assertEqual(self.harness.environment["PATH"].split(":"), [str(self.harness.stubs), str(self.harness.root / "tools")])
        self.assertEqual(sorted(path.name for path in (self.harness.root / "tools").iterdir()), sorted(TOOLS))
        for name in STUBS + MANAGERS:
            self.assertEqual(self.resolve(name), str(self.harness.stubs / name))
        for name in UNREACHABLE:
            self.assertEqual(self.resolve(name), "", name)

    def test_the_session_points_into_the_temporary_tree(self):
        for name in ("HOME", "XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_STATE_HOME", "XDG_RUNTIME_DIR", "XDG_DATA_DIRS", "KONVEYOR_SYSTEM_ROOT", "KONVEYOR_PREFIX"):
            self.assertTrue(self.harness.environment[name].startswith(str(self.harness.root)), name)
        self.assertTrue(self.harness.environment["DBUS_SESSION_BUS_ADDRESS"].startswith(f"unix:path={self.harness.root}"))

    def test_sudo_records_and_never_runs_the_command(self):
        target = self.harness.root / "touched"
        self.assertSucceeded(self.harness.run("-c", f"sudo touch {target}"))
        self.assertFalse(target.exists())
        self.assertEqual(self.harness.calls(), [["sudo", "touch", str(target)]])

    def test_runuser_keeps_the_temporary_session(self):
        result = self.assertSucceeded(self.harness.run("-c", "runuser -u tester -- env -i HOME=/root PATH=/usr/bin XDG_RUNTIME_DIR=/run/user/1000 "
                                                            "DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus USER=tester "
                                                            'sh -c \'echo "$PATH $XDG_RUNTIME_DIR $DBUS_SESSION_BUS_ADDRESS $HOME $USER"\''))
        environment = self.harness.environment
        self.assertEqual(result.stdout.split(), [environment["PATH"], environment["XDG_RUNTIME_DIR"], environment["DBUS_SESSION_BUS_ADDRESS"],
                                                 environment["HOME"], "tester"])

    def test_the_fake_kglobalaccel_refuses_keys_another_action_holds(self):
        self.assertSucceeded(self.harness.run("-c", "busctl --user call org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel "
                                                    "setShortcut asaiu 4 app act App Act 2 268435527 268435530 2"))
        self.assertEqual(self.harness.shortcut("app", "act"), ["Meta+J"])
        self.assertEqual(self.harness.shortcut("kwin", "Grid View"), ["Meta+G"])

    def test_the_fake_kwriteconfig_keeps_other_groups(self):
        self.assertSucceeded(self.harness.run("-c", "kwriteconfig6 --file kwinrc --group A --group B --key k v; "
                                                    "kwriteconfig6 --file kwinrc --group Plugins --key blurEnabled --delete"))
        self.assertEqual(self.harness.config("kwinrc"), {("A", "B"): {"k": "v"}})
        self.assertEqual(self.harness.run("-c", "kreadconfig6 --file kwinrc --group A --group B --key k").stdout, "v\n")
        self.assertEqual(self.harness.run("-c", "kreadconfig6 --file kwinrc --group A --key k --default d").stdout, "d\n")


if __name__ == "__main__":
    unittest.main()
