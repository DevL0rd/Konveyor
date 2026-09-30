#!/usr/bin/env python3
import shutil
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import HarnessTest

UNSET = {"KONVEYOR_PREFIX": ""}


class TestCustomPrefix(HarnessTest):
    def setUp(self):
        super().setUp()
        self.updater = self.harness.system / "usr" / "lib" / "konveyor"

    def resolved_prefix(self, common):
        result = self.harness.run("-c", 'source "$1"; printf %s "$KONVEYOR_PREFIX"', "prefix", str(common), env=UNSET)
        self.assertSucceeded(result)
        return result.stdout

    def install_updater(self):
        recorded = [entry.get("stdin") for entry in self.harness.entries()
                    if entry["argv"][:2] == ["sudo", "tee"] and entry["argv"][2] == str(self.updater / "install-prefix")]
        self.assertEqual(recorded, [f"{self.harness.prefix}\n"])
        for name, source in (("common.sh", "extras/packaging/common.sh"), ("updates.sh", "extras/packaging/updates.sh"),
                             ("konveyor-rebuild", "extras/packaging/konveyor-rebuild"), ("install", "install.sh")):
            self.harness.write(self.updater / name, "")
            shutil.copy2(self.harness.source / source, self.updater / name)
        self.harness.write(self.updater / "install-prefix", recorded[0])
        self.assertEqual(self.resolved_prefix(self.updater / "common.sh"), str(self.harness.prefix))

    def test_the_rebuild_hook_and_the_finishing_step_find_the_prefix_the_install_used(self):
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.install_updater()
        self.harness.write(self.harness.prefix / "share" / "konveyor" / "update-source", f"{self.harness.source}\ntester\n")
        recorder = self.updater / "install"
        shutil.move(recorder, self.harness.root / "real-install")
        recorder.write_text(f'#!/bin/sh\nprintf "%s\\n" "$*" >{self.harness.root / "rebuilt"}\n')
        recorder.chmod(0o755)
        result = self.harness.run(str(self.updater / "konveyor-rebuild"), env={**UNSET, "KONVEYOR_INSTALL_SUPPORT": str(self.updater)})
        self.assertSucceeded(result)
        self.assertEqual((self.harness.root / "rebuilt").read_text(), "--system-update\n")
        shutil.move(self.harness.root / "real-install", recorder)
        self.assertSucceeded(self.harness.run(str(recorder), "--finish-update", env={
            **UNSET, "KONVEYOR_INSTALL_SUPPORT": str(self.updater), "KONVEYOR_SOURCE_DIR": str(self.harness.source)}))
        self.assertEqual(self.harness.config("kwinrc")[("Plugins",)]["konveyor_effect_1Enabled"], "true")

    def test_uninstall_finds_the_prefix_the_install_used(self):
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertEqual(self.resolved_prefix(self.harness.source / "extras" / "packaging" / "common.sh"), str(self.harness.prefix))
        manifest = (self.harness.prefix / "share" / "konveyor" / "install_manifest.txt").read_text().split()
        self.assertSucceeded(self.harness.uninstall("--keep-widgets", env=UNSET))
        removed = {call[3] for call in self.harness.calls("sudo") if call[1:3] == ["rm", "-f"]}
        self.assertLessEqual(set(manifest), removed)

    def test_a_prefix_outside_usr_puts_its_plugins_and_qml_on_the_session_paths(self):
        self.harness.set_stub_state("systemd", {"environment": {"QT_PLUGIN_PATH": "/usr/lib/qt6/plugins"}, "enabled": []})
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        prefix = self.harness.prefix
        self.assertEqual(self.harness.stub_state("systemd")["environment"],
                         {"QT_PLUGIN_PATH": f"{prefix}/lib/plugins:/usr/lib/qt6/plugins", "QML_IMPORT_PATH": f"{prefix}/lib/qml"})
        self.assertSucceeded(self.harness.uninstall("--keep-widgets"))
        self.assertEqual(self.harness.stub_state("systemd")["environment"], {"QT_PLUGIN_PATH": "/usr/lib/qt6/plugins"})


if __name__ == "__main__":
    unittest.main()
