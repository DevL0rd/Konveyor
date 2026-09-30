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
        installer.write_text(f'#!/bin/sh\nprintf "%s\\n" "$*" "$KONVEYOR_OWNER" "$KONVEYOR_SOURCE_DIR" >{self.installed}\n')
        installer.chmod(0o755)
        self.state = self.harness.prefix / "share" / "konveyor"
        self.register(self.harness.source, "tester")
        self.harness.write(self.state / "built-for", self.harness.functions("system_fingerprint").stdout)

    def register(self, source, owner):
        self.harness.write(self.state / "update-source", f"{source}\n{owner}\n")

    def rebuild(self):
        result = self.harness.run(str(self.support / "konveyor-rebuild"), env={"KONVEYOR_INSTALL_SUPPORT": str(self.support)})
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
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

    def test_a_missing_checkout_is_skipped(self):
        self.register(self.harness.root / "gone", "tester")
        self.assertIn("is gone or not a git repository, skipping", self.rebuild())
        self.assertFalse(self.installed.exists())

    def test_nothing_happens_when_nothing_changed(self):
        self.assertEqual(self.rebuild(), "")
        self.assertFalse(self.installed.exists())

    def test_new_upstream_commits_are_merged_and_installed(self):
        self.harness.push_upstream_commit()
        log = self.rebuild()
        self.assertIn(f"konveyor: updated to {self.harness.git('rev-parse', '--short', 'origin/main')}", log)
        self.assertEqual(self.harness.git("rev-parse", "HEAD"), self.harness.git("rev-parse", "origin/main"))
        self.assertRebuilt()
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


if __name__ == "__main__":
    unittest.main()
