#!/usr/bin/env python3
import os
import subprocess
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
PACKAGING = REPO / "extras" / "packaging"

REGISTER = """
set -euo pipefail
SOURCE_DIR=$1
source "$SOURCE_DIR/extras/packaging/common.sh"
source "$SOURCE_DIR/extras/packaging/updates.sh"
KONVEYOR_ATOMIC=$2
AUR=false
SYSTEM_UPDATE_ROOT=false
mkdir -p "$KONVEYOR_STATE_DIR"
$3
echo registered
"""


def git(*arguments, cwd):
    subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@t", *arguments], cwd=cwd, check=True, capture_output=True)


class TestUpdateSource(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        root = Path(self.temporary.name)
        self.home = root / "home"
        self.home.mkdir()
        self.source = root / "source"
        (self.source / "extras").mkdir(parents=True)
        subprocess.run(["cp", "-r", str(PACKAGING), str(self.source / "extras")], check=True)
        subprocess.run(["cp", str(REPO / "install.sh"), str(self.source)], check=True)
        git("init", "-q", cwd=self.source)
        git("add", "-A", cwd=self.source)
        git("commit", "-qm", "init", cwd=self.source)
        git("clone", "-q", "--bare", str(self.source), str(root / "origin.git"), cwd=root)
        git("remote", "add", "origin", str(root / "origin.git"), cwd=self.source)
        bin_dir = root / "bin"
        bin_dir.mkdir()
        for tool in ("systemctl", "sudo"):
            stub = bin_dir / tool
            stub.write_text("#!/bin/sh\nexit 0\n")
            stub.chmod(0o755)
        self.environment = {
            **os.environ,
            "HOME": str(self.home),
            "XDG_CONFIG_HOME": str(self.home / ".config"),
            "PATH": f"{bin_dir}:{os.environ['PATH']}",
            "KONVEYOR_PREFIX": str(root / "prefix"),
        }
        self.state = root / "prefix" / "share" / "konveyor"

    def tearDown(self):
        self.temporary.cleanup()

    def register(self, atomic, command="register_updates"):
        return subprocess.run(["bash", "-c", REGISTER, "register", str(self.source), atomic, command],
                              env=self.environment, capture_output=True, text=True)

    def test_atomic_prefix_keeps_the_update_copy_and_its_state_apart(self):
        self.environment["KONVEYOR_PREFIX"] = str(self.home / ".local")
        self.state = self.home / ".local" / "share" / "konveyor"
        result = self.register("true")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("registered", result.stdout)
        copy = self.state / "source"
        self.assertTrue((copy / ".git").is_dir())
        self.assertEqual((self.state / "update-source").read_text().splitlines()[0], str(copy))

    def test_atomic_reinstall_over_an_existing_update_copy_succeeds(self):
        self.environment["KONVEYOR_PREFIX"] = str(self.home / ".local")
        self.state = self.home / ".local" / "share" / "konveyor"
        self.assertEqual(self.register("true").returncode, 0)
        result = self.register("true")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.state / "source" / ".git").is_dir())

    def test_atomic_unregister_removes_the_state_and_the_update_copy(self):
        self.environment["KONVEYOR_PREFIX"] = str(self.home / ".local")
        self.state = self.home / ".local" / "share" / "konveyor"
        self.assertEqual(self.register("true").returncode, 0)
        result = self.register("true", "unregister_updates")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.state / "update-source").exists())
        self.assertFalse((self.state / "source").exists())

    def test_rebuild_hook_reads_the_registered_state(self):
        result = subprocess.run(["bash", "-c", 'source "$1/common.sh"; source "$1/updates.sh"; printf %s "$KONVEYOR_SOURCE_STATE"',
                                 "state", str(PACKAGING)], env=self.environment, capture_output=True, text=True, check=True)
        self.assertEqual(result.stdout, str(self.state / "update-source"))
        self.assertIn('STATE_FILE=$KONVEYOR_SOURCE_STATE', (PACKAGING / "konveyor-rebuild").read_text())


if __name__ == "__main__":
    unittest.main()
