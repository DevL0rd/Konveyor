#!/usr/bin/env python3
import shutil
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import HarnessTest


class TestRebuild(HarnessTest):
    def setUp(self):
        super().setUp()
        self.support = self.harness.root / "support"
        self.support.mkdir()
        for name in ("common.sh", "updates.sh", "konveyor-rebuild"):
            shutil.copy2(self.harness.source / "extras" / "packaging" / name, self.support / name)
        self.installed = self.harness.root / "installed"
        installer = self.support / "install"
        installer.write_text(self.recorder("installed"))
        installer.chmod(0o755)
        self.state = self.harness.prefix / "share" / "konveyor"
        self.register(self.harness.source, "tester")
        self.harness.write(self.state / "built-for", self.harness.functions("system_fingerprint").stdout)

    def recorder(self, version):
        return f'#!/bin/sh\nprintf "%s\\n" "$*" "$KONVEYOR_OWNER" "$KONVEYOR_SOURCE_DIR" >{self.harness.root / version}\n'

    def register(self, source, owner):
        self.harness.write(self.state / "update-source", f"{source}\n{owner}\n")

    def rebuild(self, status=0):
        result = self.harness.run(str(self.support / "konveyor-rebuild"), env={"KONVEYOR_INSTALL_SUPPORT": str(self.support)})
        self.assertEqual(result.returncode, status, result.stdout + result.stderr)
        return result.stderr

    def assertRebuilt(self):
        self.assertEqual(self.installed.read_text().splitlines(), ["--system-update", "tester", str(self.harness.source)])

    def test_nothing_happens_without_a_registered_checkout(self):
        (self.state / "update-source").unlink()
        self.assertEqual(self.rebuild(), "")
        self.register("", "tester")
        self.assertEqual(self.rebuild(), "")
        self.register(self.harness.source, "someone-else")
        self.assertEqual(self.rebuild(), "")
        self.assertFalse(self.installed.exists())

    def test_a_gone_checkout_removes_the_update_hook_and_state(self):
        hook = self.harness.system / "usr" / "share" / "libalpm" / "hooks" / "konveyor-rebuild.hook"
        updater = self.harness.system / "usr" / "lib" / "konveyor"
        self.harness.write(hook, "")
        self.harness.write(updater / "konveyor-rebuild", "")
        self.harness.write(self.state / "build-failed", "x\n")
        self.register(self.harness.root / "gone", "tester")
        self.assertIn("is gone, removing Konveyor's system update hook", self.rebuild())
        removed = {tuple(call[1:]) for call in self.harness.calls("sudo")}
        self.assertIn(("rm", "-f", str(hook)), removed)
        self.assertIn(("rm", "-rf", str(updater)), removed)
        self.assertIn(("rm", "-f", str(self.state / "update-source")), removed)
        for name in ("built-for", "build-failed", "rebuild.lock"):
            self.assertFalse((self.state / name).exists(), name)
        self.assertFalse(self.installed.exists())

    def test_a_git_failure_in_an_existing_checkout_is_skipped_not_removed(self):
        broken = self.harness.root / "broken"
        broken.mkdir()
        self.register(broken, "tester")
        self.assertIn("is not a readable git repository, skipping", self.rebuild())
        self.assertEqual(self.harness.calls("sudo"), [])
        self.assertTrue((self.state / "built-for").exists())
        self.assertFalse(self.installed.exists())

    def test_a_gone_checkout_is_only_skipped_when_the_home_is_missing(self):
        hook = self.harness.system / "usr" / "share" / "libalpm" / "hooks" / "konveyor-rebuild.hook"
        self.harness.write(hook, "")
        self.register(self.harness.root / "gone", "tester")
        self.harness.environment["STUB_OWNER_HOME"] = str(self.harness.root / "unmounted-home")
        self.assertIn("the home of tester is missing, skipping", self.rebuild())
        self.assertEqual(self.harness.calls("sudo"), [])
        self.assertTrue((self.state / "built-for").exists())
        self.assertFalse(self.installed.exists())

    def test_nothing_happens_when_nothing_changed(self):
        self.assertEqual(self.rebuild(), "")
        self.assertFalse(self.installed.exists())

    def test_new_upstream_commits_are_merged_and_installed_by_their_own_installer(self):
        self.harness.push_upstream_commit("install.sh", self.recorder("pulled"))
        log = self.rebuild()
        self.assertIn(f"konveyor: updated to {self.harness.git('rev-parse', '--short', 'origin/main')}", log)
        self.assertEqual(self.harness.git("rev-parse", "HEAD"), self.harness.git("rev-parse", "origin/main"))
        self.assertFalse(self.installed.exists())
        self.assertEqual((self.harness.root / "pulled").read_text().splitlines(), ["--system-update", "tester", str(self.harness.source)])
        git = [call for call in self.harness.calls("runuser")]
        self.assertTrue(all(call[:4] == ["runuser", "-u", "tester", "--"] for call in git))

    def test_local_changes_skip_the_pull(self):
        self.harness.push_upstream_commit()
        (self.harness.source / "install.sh").write_text("changed\n")
        self.assertIn("has local changes, skipping the update", self.rebuild())
        self.assertFalse(self.installed.exists())

    def test_local_commits_skip_the_pull(self):
        self.harness.push_upstream_commit()
        (self.harness.source / "local.txt").write_text("local")
        self.harness.git("add", "local.txt")
        self.harness.git("commit", "-qm", "Local")
        self.assertIn("has local commits that aren't upstream, skipping the update", self.rebuild())
        self.assertFalse(self.installed.exists())

    def test_an_unreachable_remote_is_reported(self):
        self.harness.git("remote", "set-url", "origin", str(self.harness.root / "gone.git"))
        self.assertIn("could not fetch updates", self.rebuild())
        self.assertFalse(self.installed.exists())

    def test_a_system_change_rebuilds_without_new_commits(self):
        (self.state / "built-for").write_text("kwin=6.6.0\n")
        self.rebuild()
        self.assertRebuilt()

    def test_a_kwin_update_waits_for_its_development_files(self):
        self.harness.set_headers("6.6.0")
        (self.state / "built-for").write_text("kwin=6.6.0\n")
        self.assertIn("KWin 6.7.0 and its development files 6.6.0 don't match yet", self.rebuild())
        self.assertFalse(self.installed.exists())

    def test_pulled_commits_are_not_lost_while_waiting_for_the_development_files(self):
        self.harness.set_headers("6.6.0")
        self.harness.push_upstream_commit("install.sh", self.recorder("pulled"))
        self.assertIn("don't match yet", self.rebuild())
        self.assertFalse((self.state / "built-for").exists())
        self.harness.set_headers("6.7.0")
        self.rebuild()
        self.assertRebuilt()

    def failing_installer(self):
        installer = self.support / "install"
        installer.write_text(self.recorder("installed").replace('" >', '" >>') + "exit 1\n")
        self.installed.write_text("")

    def attempts(self):
        return self.installed.read_text().count("--system-update")

    def test_a_failed_build_is_not_repeated_for_the_same_inputs(self):
        (self.state / "built-for").write_text("kwin=6.6.0\n")
        self.failing_installer()
        self.assertIn("the build failed", self.rebuild(1))
        self.assertEqual(self.attempts(), 1)
        self.assertIn("last build failed for these inputs, skipping; run ./install.sh to retry", self.rebuild())
        self.assertEqual(self.attempts(), 1)

    def test_a_failed_build_is_retried_when_the_inputs_change(self):
        (self.state / "built-for").write_text("kwin=6.6.0\n")
        self.failing_installer()
        self.rebuild(1)
        self.harness.write(self.state / "build-failed", "other\ninputs\n")
        self.rebuild(1)
        self.assertEqual(self.attempts(), 2)

    def test_recording_the_fingerprint_clears_a_failed_build(self):
        self.harness.write(self.state / "build-failed", "x\n")
        result = self.harness.functions("record_fingerprint")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(["sudo", "rm", "-f", str(self.state / "build-failed")], self.harness.calls("sudo"))

    def test_a_running_rebuild_holds_the_lock(self):
        (self.state / "built-for").write_text("kwin=6.6.0\n")
        result = self.harness.run("-c", f'exec 9>"{self.state}/rebuild.lock"; flock 9; timeout 1 {self.support}/konveyor-rebuild; echo $?',
                                  env={"KONVEYOR_INSTALL_SUPPORT": str(self.support)})
        self.assertEqual(result.stdout.strip().splitlines()[-1], "124")
        self.assertFalse(self.installed.exists())


if __name__ == "__main__":
    unittest.main()
