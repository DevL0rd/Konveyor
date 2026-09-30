#!/usr/bin/env python3
import shutil
import sys
import tarfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import REPO, HarnessTest

COMMON = Path("widgets") / "shared" / "common"


class TestReleaseArchive(HarnessTest):
    checkout = False

    def setUp(self):
        super().setUp()
        root = self.harness.root
        self.common = root / "common.git"
        shutil.copytree(REPO / COMMON, self.common, ignore=shutil.ignore_patterns(".git"))
        self.harness.git("init", "-q", cwd=self.common)
        self.harness.git("add", "-A", cwd=self.common)
        self.harness.git("commit", "-qm", "Common", cwd=self.common)
        self.release = root / "release"
        shutil.copytree(self.harness.source, self.release)
        shutil.rmtree(self.release / COMMON)
        (self.release / "tools").mkdir()
        shutil.copy2(REPO / "tools" / "source-archive.sh", self.release / "tools" / "source-archive.sh")
        self.harness.git("init", "-q", cwd=self.release)
        self.harness.git("submodule", "add", "-q", str(self.common), str(COMMON), cwd=self.release)
        self.harness.git("add", "-A", cwd=self.release)
        self.harness.git("commit", "-qm", "Release", cwd=self.release)

    def unpack(self, archive):
        with tarfile.open(archive) as tar:
            names = tar.getnames()
            tar.extractall(self.harness.root / "unpacked", filter="data")
        return names

    def install(self):
        return self.harness.run(str(self.harness.root / "unpacked" / "konveyor-1.2.3" / "install.sh"), "--skip-deps")

    def test_the_release_archive_carries_the_submodules_and_installs_the_widgets(self):
        result = self.assertSucceeded(self.harness.run(str(self.release / "tools" / "source-archive.sh"), "1.2.3", str(self.harness.root)))
        archive = self.harness.root / "konveyor-1.2.3.tar.gz"
        self.assertEqual(result.stdout, f"{archive}\n")
        names = self.unpack(archive)
        self.assertTrue(all(name.startswith("konveyor-1.2.3/") for name in names))
        self.assertIn("konveyor-1.2.3/widgets/shared/common/FileWatcher.qml", names)
        self.assertFalse([name for name in names if "/.git/" in name or name.endswith("/.git")])
        result = self.assertSucceeded(self.install())
        self.assertIn("Konveyor widgets are installed", result.stdout)
        runtime = self.harness.home / ".local" / "share" / "konveyor" / "widgets"
        self.assertTrue((runtime / "shared" / "common" / "FileWatcher.qml").exists())
        self.assertEqual(self.harness.shortcut("konveyor-kontrol-panel", "toggle"), ["Meta", "Alt+F1"])

    def test_a_plain_git_archive_leaves_the_widgets_without_their_shared_files(self):
        archive = self.harness.root / "plain.tar.gz"
        self.harness.git("archive", "--format=tar.gz", "--prefix=konveyor-1.2.3/", "-o", str(archive), "HEAD", cwd=self.release)
        self.unpack(archive)
        result = self.install()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("widgets/shared/common is empty", result.stderr)

    def test_an_archive_needs_the_submodules_checked_out(self):
        self.harness.git("submodule", "deinit", "--force", str(COMMON), cwd=self.release)
        result = self.harness.run(str(self.release / "tools" / "source-archive.sh"), "1.2.3", str(self.harness.root))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("run: git submodule update --init --recursive", result.stderr)
        self.assertFalse((self.harness.root / "konveyor-1.2.3.tar.gz").exists())

    def test_the_version_is_required(self):
        result = self.harness.run(str(self.release / "tools" / "source-archive.sh"))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Usage: ./tools/source-archive.sh <version>", result.stderr)

    def test_the_release_job_builds_the_archive_with_the_submodules(self):
        workflow = (REPO / ".github" / "workflows" / "ci.yml").read_text()
        release = workflow[workflow.index("\n  release:"):]
        self.assertIn("submodules: recursive", release)
        self.assertIn('tools/source-archive.sh "$version"', release)
        self.assertNotIn("git archive", release)


if __name__ == "__main__":
    unittest.main()
