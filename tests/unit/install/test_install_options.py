#!/usr/bin/env python3
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import HarnessTest

STATE = Path(".local") / "state" / "konveyor"


class TestInstallOptions(HarnessTest):
    def plugins(self):
        return self.harness.config("kwinrc")[("Plugins",)]

    def test_a_second_install_changes_nothing(self):
        self.assertSucceeded(self.harness.install("--skip-deps"))
        before = self.harness.snapshot()
        first = self.harness.calls("sudo")
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.assertEqual(self.harness.snapshot(), before)
        self.assertEqual(self.harness.calls("sudo"), first)

    def test_install_enables_the_effect_and_the_telemetry(self):
        result = self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertEqual(self.plugins(), {"blurEnabled": "true", "konveyor_effectEnabled": "false",
                                          "konveyor_effect_1700000000Enabled": "true"})
        self.assertEqual(self.harness.stub_state("kwin")["loaded"], ["konveyor_effect_1700000000"])
        self.assertIn("Konveyor is live now", result.stdout)
        self.assertIn("Settings: open System Settings", result.stdout)
        self.assertEqual((self.harness.home / STATE / "install-options").read_text(), f"widgets=false\nprefix={self.harness.prefix}\n")
        self.assertFalse((self.harness.home / ".local" / "share" / "konveyor" / "widgets").exists())
        self.assertEqual(self.harness.calls("busctl"), [])

    def test_switching_to_no_widgets_turns_the_telemetry_off(self):
        self.assertSucceeded(self.harness.install("--skip-deps"))
        self.assertEqual(self.plugins()["process_monitor_telemetry_1700000000Enabled"], "true")
        self.assertEqual((self.harness.home / STATE / "install-options").read_text(), f"widgets=true\nprefix={self.harness.prefix}\n")
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertNotIn("process_monitor_telemetry_1700000000Enabled", self.plugins())
        self.assertEqual(self.harness.stub_state("kwin")["loaded"], ["konveyor_effect_1700000000"])

    def test_a_refused_effect_asks_for_a_new_login(self):
        self.harness.set_stub_state("kwin", {"running": True, "loaded": [], "refuse": ["konveyor_effect_1700000000"]})
        result = self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertIn("Log out and back in to start it", result.stdout)

    def test_dependencies_are_installed_unless_skipped(self):
        self.assertSucceeded(self.harness.install("--no-widgets"))
        self.assertEqual([call[:5] for call in self.harness.calls("sudo") if call[1] == "pacman"],
                         [["sudo", "pacman", "-S", "--needed", "--noconfirm"]])
        self.assertSucceeded(self.harness.install("--no-widgets", "--skip-deps"))
        self.assertEqual([call for call in self.harness.calls("sudo") if call[1] == "pacman"], [])

    def test_install_pulls_the_checkout_unless_told_not_to(self):
        self.harness.push_upstream_commit()
        head = self.harness.git("rev-parse", "HEAD")
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets", "--no-pull"))
        self.assertEqual(self.harness.git("rev-parse", "HEAD"), head)
        result = self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertIn("Updating source checkout", result.stdout)
        self.assertEqual(self.harness.git("rev-parse", "HEAD"), self.harness.git("rev-parse", "origin/main"))

    def test_a_branch_without_upstream_is_not_pulled(self):
        self.harness.git("checkout", "-q", "-b", "local")
        result = self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertNotIn("Updating source checkout", result.stdout)

    def test_a_diverged_checkout_stops_the_install(self):
        self.harness.push_upstream_commit()
        (self.harness.source / "local.txt").write_text("local")
        self.harness.git("add", "local.txt")
        self.harness.git("commit", "-qm", "Local")
        result = self.harness.install("--skip-deps", "--no-widgets")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.harness.calls("cmake"), [])

    def test_aur_installs_register_no_update_hook(self):
        for arguments, environment in ((["--aur"], {}), ([], {"KONVEYOR_AUR": "true"}), ([], {"KONVEYOR_AUR": "yes"})):
            with self.subTest(arguments=arguments, environment=environment):
                self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets", *arguments, env=environment))
                installs = [call[-1] for call in self.harness.calls("sudo") if call[1] == "install"]
                self.assertFalse([target for target in installs if "libalpm" in target or target.endswith("/install")])
                self.assertFalse((self.harness.home / ".local" / "share" / "konveyor" / "source").exists())
                self.assertFalse((self.harness.home / ".config" / "systemd" / "user" / "konveyor-update.service").exists())

    def test_an_aur_install_removes_an_earlier_update_hook(self):
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertTrue((self.harness.home / ".config" / "systemd" / "user" / "konveyor-update.service").exists())
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets", "--aur"))
        self.assertFalse((self.harness.home / ".config" / "systemd" / "user" / "konveyor-update.service").exists())
        self.assertFalse((self.harness.home / ".local" / "share" / "konveyor" / "source").exists())
        self.assertNotIn("konveyor-update.service", self.harness.stub_state("systemd")["enabled"])

    def test_options(self):
        self.assertIn("--no-widgets", self.assertSucceeded(self.harness.install("--help")).stdout)
        result = self.harness.install("--bogus")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unknown option: --bogus", result.stderr)
        result = self.harness.install("--skip-deps", root=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("run install.sh as your normal user", result.stderr)
        self.assertEqual(self.harness.calls("cmake"), [])


class TestWidgetInstallerOptions(HarnessTest):
    def widgets(self, script, *arguments, root=False):
        return self.harness.run(str(self.harness.source / "widgets" / script), *arguments, root=root)

    def test_options(self):
        self.assertIn("--no-restart", self.assertSucceeded(self.widgets("install.sh", "--help")).stdout)
        result = self.widgets("install.sh", "--bogus")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unknown option: --bogus", result.stderr)
        for script in ("install.sh", "uninstall.sh"):
            result = self.widgets(script, root=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("as your normal user", result.stderr)

    def test_missing_commands_are_named(self):
        for name in ("kscreen-doctor", "jq"):
            (self.harness.stubs / name).unlink()
        result = self.widgets("install.sh", "--no-restart")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("missing commands for the widgets: kscreen-doctor jq", result.stderr)


class TestInstallFromAnArchive(HarnessTest):
    checkout = False

    def test_an_archive_install_registers_no_updates(self):
        result = self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertNotIn("Registering Konveyor with system updates", result.stdout)
        self.assertFalse((self.harness.home / ".config" / "systemd" / "user" / "konveyor-update.service").exists())


class TestBuild(HarnessTest):
    def build(self, prefix):
        built_for = self.harness.root / "built-for"
        self.assertSucceeded(self.harness.functions(f"KONVEYOR_PREFIX={prefix}; KONVEYOR_BUILT_FOR={built_for}; build"))
        return self.harness.calls("cmake")

    def test_the_build_is_clean_only_when_the_system_changed(self):
        self.assertIn("--clean-first", self.build(self.harness.prefix)[1])
        self.harness.write(self.harness.root / "built-for", self.harness.functions("system_fingerprint").stdout)
        self.assertNotIn("--clean-first", self.build(self.harness.prefix)[1])
        self.harness.write(self.harness.system / "usr" / "lib" / "libQt6Core.so.6.10.0", "")
        (self.harness.system / "usr" / "lib" / "libQt6Core.so.6").unlink()
        (self.harness.system / "usr" / "lib" / "libQt6Core.so.6").symlink_to("libQt6Core.so.6.10.0")
        self.assertIn("--clean-first", self.build(self.harness.prefix)[1])

    def test_the_fingerprint_names_what_the_build_depends_on(self):
        self.assertEqual(self.harness.functions("system_fingerprint").stdout.splitlines()[:2], ["kwin=6.7.0", "qt=6.9.1"])

    def test_a_usr_prefix_uses_the_system_qt_paths(self):
        self.assertIn("-DKDE_INSTALL_USE_QT_SYS_PATHS=ON", self.build("/usr")[0])
        self.assertIn("-DKDE_INSTALL_USE_QT_SYS_PATHS=OFF", self.build(self.harness.prefix)[0])
        self.assertIn(f"-DCMAKE_INSTALL_PREFIX={self.harness.prefix}", self.build(self.harness.prefix)[0])


if __name__ == "__main__":
    unittest.main()
