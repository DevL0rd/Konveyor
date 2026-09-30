#!/usr/bin/env python3
import subprocess
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import REPO, Harness, HarnessTest

PACKAGING = REPO / "extras" / "packaging"
REBUILD = "/usr/lib/konveyor/konveyor-rebuild"
HOOKS = {
    "pacman": ("konveyor-rebuild.hook", "usr/share/libalpm/hooks/konveyor-rebuild.hook", "644"),
    "dnf": ("konveyor-rebuild.actions", "etc/dnf/libdnf5-plugins/actions.d/konveyor-rebuild.actions", "644"),
    "zypper": ("konveyor-rebuild.zypp", "usr/lib/zypp/plugins/commit/konveyor-rebuild", "755"),
    "apt-get": ("konveyor-rebuild.apt", "etc/apt/apt.conf.d/99konveyor-rebuild", "644"),
}
UPDATER = ("install", "common.sh", "updates.sh", "konveyor-rebuild")


class TestPackageManagers(unittest.TestCase):
    def harness(self, *managers):
        harness = Harness(managers)
        self.addCleanup(harness.close)
        return harness

    def test_each_package_manager_gets_its_own_hook(self):
        for manager, (source, target, mode) in HOOKS.items():
            with self.subTest(manager=manager):
                harness = self.harness(manager)
                result = harness.functions("register_updates")
                self.assertEqual(result.returncode, 0, result.stderr)
                calls = harness.calls("sudo")
                updater = harness.system / "usr" / "lib" / "konveyor"
                installed = [(call[2], call[4]) for call in calls if call[1] == "install"]
                self.assertEqual(installed[:4], [("-Dm755" if name in ("install", "konveyor-rebuild") else "-Dm644", str(updater / name))
                                                 for name in UPDATER])
                self.assertEqual(calls[-1 if manager != "pacman" else -2][1:],
                                 ["install", f"-Dm{mode}", str(harness.source / "extras" / "packaging" / source), str(harness.system / target)])
                edits = [call for call in calls if call[1] == "sed"]
                self.assertEqual(len(edits), 1 if manager == "pacman" else 0)
                unit = (harness.home / ".config" / "systemd" / "user" / "konveyor-update.service").read_text()
                self.assertIn(f"Environment=KONVEYOR_SOURCE_DIR={harness.home}/.local/share/konveyor/source\n", unit)
                self.assertIn("konveyor-update.service", harness.stub_state("systemd")["enabled"])

    def test_the_first_package_manager_found_wins(self):
        for managers, expected in ((("apt-get", "zypper", "dnf", "pacman"), "pacman"), (("apt-get", "zypper", "dnf"), "dnf"),
                                   (("apt-get", "zypper"), "zypper"), (("apt-get",), "apt-get")):
            with self.subTest(managers=managers):
                result = self.harness(*managers).functions("package_manager")
                self.assertEqual(result.stdout.strip(), expected)

    def test_no_package_manager_fails_visibly(self):
        harness = self.harness()
        self.assertEqual(harness.functions("package_manager").returncode, 1)
        result = harness.functions("register_updates")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("no supported package manager", result.stderr)

    def test_dependencies_come_from_the_package_manager(self):
        for manager, command in (("pacman", ["pacman", "-S", "--needed", "--noconfirm", "base-devel"]), ("dnf", ["dnf", "install", "-y", "cmake"]),
                                 ("zypper", ["zypper", "--non-interactive", "install", "cmake"]), ("apt-get", ["apt-get", "install", "-y", "cmake"])):
            with self.subTest(manager=manager):
                harness = self.harness(manager, "apt-get")
                result = harness.run(str(harness.source / "extras" / "packaging" / "dependencies.sh"))
                self.assertEqual(result.returncode, 0, result.stderr)
                calls = harness.calls("sudo")
                self.assertEqual(len(calls), 1)
                self.assertEqual(calls[0][1:len(command) + 1], command)
        harness = self.harness()
        result = harness.run(str(harness.source / "extras" / "packaging" / "dependencies.sh"))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unsupported package manager", result.stderr)

    def test_uninstall_keeps_directories_a_package_owns(self):
        for manager in HOOKS:
            with self.subTest(manager=manager):
                harness = self.harness(manager)
                owned, free = harness.prefix / "lib" / "shared", harness.prefix / "share" / "ours"
                for directory in (owned, free):
                    directory.mkdir(parents=True)
                manifest = harness.root / "manifest"
                manifest.write_text(f"{owned}/plugin.so\n{free}/file")
                harness.set_stub_state("packages", {"system": {}, "box": {}, "owned": [str(owned)], "groups": {}, "repositories": {}})
                result = harness.functions(f"remove_empty_directories {manifest}", script="uninstall.sh")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(harness.calls("sudo"), [["sudo", "rmdir", str(free)]])


class TestHooks(HarnessTest):
    def test_the_pacman_hook_lets_the_rebuild_reach_the_network_when_pacman_can(self):
        self.assertSucceeded(self.harness.functions("register_updates"))
        edit = [call for call in self.harness.calls("sudo") if call[1] == "sed"][0]
        hook = subprocess.run(["sed", edit[3], str(PACKAGING / "konveyor-rebuild.hook")], capture_output=True, text=True, check=True).stdout
        self.assertIn(f"Exec = {REBUILD}\nNetworkAccess = allowed\nDepends = cmake\n", hook)
        (self.harness.system / "usr" / "lib" / "libalpm.so.15").write_text("older pacman\n")
        self.assertSucceeded(self.harness.functions("register_updates"))
        self.assertEqual([call for call in self.harness.calls("sudo") if call[1] == "sed"], [])

    def test_the_hooks_run_the_updater_that_gets_installed(self):
        result = subprocess.run(["bash", "-c", 'source "$1/common.sh"; source "$1/updates.sh"; printf %s "$KONVEYOR_UPDATER_DIR"', "dir",
                                 str(PACKAGING)], env={"HOME": str(self.harness.home), "PATH": self.harness.environment["PATH"]},
                                capture_output=True, text=True, check=True)
        self.assertEqual(result.stdout + "/konveyor-rebuild", REBUILD)
        self.assertIn(f"Exec = {REBUILD}\n", (PACKAGING / "konveyor-rebuild.hook").read_text())
        self.assertEqual((PACKAGING / "konveyor-rebuild.actions").read_text().strip().split(":"),
                         ["post_transaction", "", "", "enabled=host-only raise_error=0", REBUILD])
        self.assertIn(f"ExecStart={result.stdout}/install --finish-update", (PACKAGING / "konveyor-update.service.in").read_text())

    def rebuild(self, exit_code):
        script = self.harness.root / "konveyor-rebuild"
        script.write_text(f"#!/bin/sh\necho ran >>{self.harness.root / 'rebuilt'}\nexit {exit_code}\n")
        script.chmod(0o755)
        return script

    def test_the_apt_hook_never_fails_apt(self):
        line = (PACKAGING / "konveyor-rebuild.apt").read_text()
        command = line.split('"', 1)[1].rsplit('"', 1)[0]
        for exit_code in (0, 1):
            with self.subTest(exit_code=exit_code):
                script = self.rebuild(exit_code)
                result = self.harness.run("-c", command.replace(REBUILD, str(script)))
                self.assertEqual(result.returncode, 0)
        (self.harness.root / "konveyor-rebuild").unlink()
        self.assertEqual(self.harness.run("-c", command.replace(REBUILD, str(self.harness.root / "konveyor-rebuild"))).returncode, 0)
        self.assertEqual((self.harness.root / "rebuilt").read_text(), "ran\nran\n")

    def zypp(self, frames, rebuild):
        plugin = self.harness.root / "zypp-plugin"
        plugin.write_text((PACKAGING / "konveyor-rebuild.zypp").read_text().replace(REBUILD, str(rebuild)))
        return subprocess.run([str(self.harness.root / "tools" / "bash"), str(plugin)], input="".join(f"{frame}\n\n\0" for frame in frames),
                              env=self.harness.environment, capture_output=True, text=True)

    def test_the_zypper_plugin_acknowledges_every_frame_and_rebuilds_after_the_commit(self):
        frames = ["PLUGINBEGIN", "COMMITBEGIN\nuserdata:x", "COMMITEND", "PLUGINEND", "_DISCONNECT", "NEVERREAD"]
        result = self.zypp(frames, self.rebuild(0))
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "ACK\n\n\0" * 5)
        self.assertEqual(self.harness.calls(), [["systemd-run", "--no-block", "--collect", "--unit=konveyor-rebuild", str(self.harness.root / "konveyor-rebuild")]])

    def test_the_zypper_plugin_skips_the_rebuild_once_konveyor_is_gone(self):
        result = self.zypp(["COMMITEND", "_DISCONNECT"], self.harness.root / "missing")
        self.assertEqual(result.stdout, "ACK\n\n\0" * 2)
        self.assertEqual(self.harness.calls(), [])

    def test_unregister_removes_every_hook_and_the_update_copy(self):
        self.assertSucceeded(self.harness.functions("register_updates"))
        for source, target, mode in HOOKS.values():
            self.harness.write(self.harness.system / target, "")
        legacy = self.harness.system / "etc" / "pacman.d" / "hooks" / "konveyor-rebuild.hook"
        self.harness.write(legacy, "")
        state = self.harness.prefix / "share" / "konveyor"
        for name in ("update-source", "source"):
            self.harness.write(state / name, "")
        self.harness.write(self.harness.system / "usr" / "lib" / "konveyor" / "install", "")
        self.harness.calls()
        self.assertSucceeded(self.harness.functions("unregister_updates"))
        removed = {tuple(call[1:]) for call in self.harness.calls("sudo")}
        for source, target, mode in HOOKS.values():
            self.assertIn(("rm", "-f", str(self.harness.system / target)), removed)
        self.assertIn(("rm", "-f", str(legacy)), removed)
        self.assertIn(("rm", "-rf", str(self.harness.system / "usr" / "lib" / "konveyor")), removed)
        self.assertIn(("rm", "-f", str(state / "update-source")), removed)
        self.assertIn(("rm", "-f", str(state / "source")), removed)
        self.assertFalse((self.harness.home / ".config" / "systemd" / "user" / "konveyor-update.service").exists())
        self.assertFalse((self.harness.home / ".local" / "share" / "konveyor").exists())
        self.assertEqual(self.harness.stub_state("systemd")["enabled"], [])

    def test_register_removes_the_legacy_hook_and_state(self):
        legacy = self.harness.system / "etc" / "pacman.d" / "hooks" / "konveyor-rebuild.hook"
        self.harness.write(legacy, "")
        self.harness.write(self.harness.prefix / "share" / "konveyor" / "source", "")
        self.assertSucceeded(self.harness.functions("register_updates"))
        entries = self.harness.entries()
        removed = [entry["argv"][1:] for entry in entries if entry["argv"][:2] == ["sudo", "rm"]]
        self.assertIn(["rm", "-f", str(legacy)], removed)
        self.assertIn(["rm", "-f", str(self.harness.prefix / "share" / "konveyor" / "source")], removed)
        state = [entry["stdin"] for entry in entries if entry["argv"][:2] == ["sudo", "tee"]]
        self.assertEqual(state, [f"{self.harness.home}/.local/share/konveyor/source\ntester\n"])

    def test_the_update_copy_clones_over_https(self):
        for remote, expected in (("git@github.com:DevL0rd/Konveyor.git", "https://github.com/DevL0rd/Konveyor.git"),
                                 ("ssh://git@github.com/DevL0rd/Konveyor.git", "https://github.com/DevL0rd/Konveyor.git"),
                                 ("https://example.org/Konveyor.git", "https://example.org/Konveyor.git")):
            with self.subTest(remote=remote):
                self.harness.git("remote", "set-url", "origin", remote)
                self.assertEqual(self.harness.functions("update_source_url").stdout, f"{expected}\n")
        self.harness.git("remote", "remove", "origin")
        result = self.harness.functions("register_updates")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("has no origin remote to keep an update copy of Konveyor from", result.stderr)

    def test_the_update_copy_follows_the_remote_branch(self):
        self.assertSucceeded(self.harness.functions("register_updates"))
        copy = self.harness.home / ".local" / "share" / "konveyor" / "source"
        (copy / "install.sh").write_text("local edit\n")
        self.harness.push_upstream_commit()
        self.assertSucceeded(self.harness.functions("register_updates"))
        self.assertEqual(self.harness.git("rev-parse", "HEAD", cwd=copy), self.harness.git("rev-parse", "main", cwd=self.harness.root / "origin.git"))
        self.assertEqual((copy / "install.sh").read_text(), (self.harness.source / "install.sh").read_text())

    def test_a_system_update_registers_no_user_unit(self):
        self.assertSucceeded(self.harness.functions("SYSTEM_UPDATE_ROOT=true; register_updates"))
        self.assertFalse((self.harness.home / ".config" / "systemd" / "user" / "konveyor-update.service").exists())


if __name__ == "__main__":
    unittest.main()
