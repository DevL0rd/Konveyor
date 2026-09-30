#!/usr/bin/env python3
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import HarnessTest

STATE = Path(".local") / "state" / "konveyor"


class UpdateTest(HarnessTest):
    def setUp(self):
        super().setUp()
        self.state = self.harness.home / STATE
        self.harness.write(self.state / "update-pending", "")

    def options(self, widgets):
        self.harness.write(self.state / "install-options", f"widgets={widgets}\n")

    def mark_built(self):
        self.harness.write(self.harness.prefix / "share" / "konveyor" / "built-for", self.harness.functions("system_fingerprint").stdout)
        self.harness.calls()

    def notifications(self, calls):
        return [call[call.index("system-software-update") + 1] for call in calls if "org.freedesktop.Notifications.Notify" in call]


class TestFinishUpdate(UpdateTest):
    def finish(self):
        return self.assertSucceeded(self.harness.install("--finish-update", env={"KONVEYOR_SOURCE_DIR": str(self.harness.source)}))

    def test_finishing_without_widgets_enables_the_new_plugin(self):
        self.options("false")
        self.finish()
        self.assertEqual(self.harness.config("kwinrc")[("Plugins",)]["konveyor_effect_1Enabled"], "true")
        self.assertNotIn("process_monitor_telemetry_1Enabled", self.harness.config("kwinrc")[("Plugins",)])
        self.assertFalse((self.state / "update-pending").exists())
        self.assertFalse((self.harness.home / ".local" / "share" / "konveyor" / "widgets").exists())
        self.assertEqual(self.notifications(self.harness.calls()), ["Konveyor updated"])

    def test_finishing_updates_the_widgets_without_restarting_plasma(self):
        self.options("true")
        self.finish()
        self.assertEqual(self.harness.config("kwinrc")[("Plugins",)]["process_monitor_telemetry_1Enabled"], "true")
        self.assertTrue((self.harness.home / ".local" / "share" / "konveyor" / "widgets" / "install.sh").exists())
        self.assertNotIn(["systemctl", "--user", "stop", "plasma-plasmashell.service"], self.harness.calls("systemctl"))

    def test_finishing_runs_the_new_widget_installer_so_new_steps_like_the_kontrol_panel_service_happen(self):
        self.options("true")
        previous = self.harness.home / ".local" / "share" / "konveyor" / "widgets" / "install.sh"
        self.harness.write(previous, '#!/bin/sh\necho "previous widget installer ran"\n')
        previous.chmod(0o755)
        result = self.finish()
        self.assertNotIn("previous widget installer ran", result.stdout)
        self.assertTrue((self.harness.home / ".config" / "systemd" / "user" / "konveyor-kontrol-panel.service").exists())


class TestSystemUpdate(UpdateTest):
    def system_update(self, **environment):
        return self.harness.install("--system-update", root=True, env={"KONVEYOR_SOURCE_DIR": str(self.harness.source), **environment})

    def test_it_only_runs_as_root_for_an_owner(self):
        for root, environment in ((False, {"KONVEYOR_OWNER": "tester"}), (True, {})):
            result = self.harness.install("--system-update", root=root, env=environment)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("--system-update runs from the system update hook", result.stderr)
        self.assertEqual(self.harness.calls("cmake"), [])

    def test_it_rebuilds_and_finishes_in_the_running_session(self):
        self.options("false")
        self.harness.start_session()
        result = self.system_update(KONVEYOR_OWNER="tester")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        calls = self.harness.calls()
        builds = [call for call in calls if call[0] == "runuser" and "cmake" in call]
        self.assertEqual([call[call.index("cmake") + 1] for call in builds], ["-S", "--build"])
        finish = [call for call in calls if call[0] == "runuser" and "--finish-update" in call]
        self.assertEqual(finish[0][-2:], [str(self.harness.system / "usr" / "lib" / "konveyor" / "install"), "--finish-update"])
        self.assertTrue((self.harness.system / "usr" / "lib" / "konveyor" / "konveyor-rebuild").exists())
        self.assertEqual((self.harness.prefix / "share" / "konveyor" / "plugin-id").read_text(), "konveyor_effect_1700000000\n")
        self.assertEqual(self.harness.config("kwinrc")[("Plugins",)]["konveyor_effect_1700000000Enabled"], "true")
        self.assertFalse((self.state / "update-pending").exists())
        self.assertFalse((self.harness.home / ".config" / "systemd" / "user" / "konveyor-update.service").exists())

    def test_an_install_without_widgets_gets_no_frame_telemetry_plugin_from_an_update(self):
        self.options("false")
        self.harness.start_session()
        self.assertSucceeded(self.system_update(KONVEYOR_OWNER="tester"))
        plugins = self.harness.prefix / "lib" / "plugins" / "kwin" / "effects" / "plugins"
        self.assertEqual(sorted(path.name for path in plugins.iterdir()), ["konveyor_effect_1700000000.so"])

    def test_without_a_session_it_leaves_the_rest_for_the_next_login(self):
        (self.state / "update-pending").unlink()
        result = self.system_update(KONVEYOR_OWNER="tester")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("its session steps run at your next login", result.stdout)
        self.assertTrue((self.state / "update-pending").exists())
        self.assertFalse([call for call in self.harness.calls() if "--finish-update" in call])


class TestLoginUpdate(UpdateTest):
    def login_update(self):
        return self.assertSucceeded(self.harness.install("--login-update", env={"KONVEYOR_SOURCE_DIR": str(self.harness.source)}))

    def test_nothing_happens_when_nothing_changed(self):
        self.mark_built()
        self.login_update()
        self.assertEqual(self.harness.calls("cmake"), [])

    def test_a_changed_system_rebuilds(self):
        self.options("false")
        self.login_update()
        calls = self.harness.calls()
        self.assertEqual(self.notifications(calls), ["Updating Konveyor", "Konveyor updated"])
        self.assertEqual([call[1] for call in calls if call[0] == "cmake"], ["-S", "--build"])
        self.assertIn(["sudo", "pacman", "-S", "--needed", "--noconfirm"], [call[:5] for call in calls if call[0] == "sudo"])

    def test_new_upstream_commits_are_pulled_and_built(self):
        self.options("false")
        self.mark_built()
        self.harness.push_upstream_commit()
        self.login_update()
        self.assertEqual(self.harness.git("rev-parse", "HEAD"), self.harness.git("rev-parse", "origin/main"))
        self.assertEqual([call[1] for call in self.harness.calls("cmake")], ["-S", "--build"])

    def test_local_changes_and_commits_are_never_overwritten(self):
        self.mark_built()
        self.harness.push_upstream_commit()
        head = self.harness.git("rev-parse", "HEAD")
        (self.harness.source / "install.sh").write_text((self.harness.source / "install.sh").read_text() + "\n")
        self.login_update()
        self.assertEqual(self.harness.git("rev-parse", "HEAD"), head)
        self.harness.git("commit", "-qam", "Local")
        head = self.harness.git("rev-parse", "HEAD")
        self.login_update()
        self.assertEqual(self.harness.git("rev-parse", "HEAD"), head)
        self.assertEqual(self.harness.calls("cmake"), [])

    def test_an_unreachable_remote_is_reported(self):
        self.mark_built()
        self.harness.git("remote", "set-url", "origin", str(self.harness.root / "gone.git"))
        result = self.login_update()
        self.assertIn(f"Could not fetch updates for {self.harness.source}", result.stdout)
        self.assertEqual(self.harness.calls("cmake"), [])


if __name__ == "__main__":
    unittest.main()
