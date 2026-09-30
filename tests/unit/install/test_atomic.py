#!/usr/bin/env python3
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import HarnessTest

MIRROR = "https://steamdeck-packages.steamos.cloud/archlinux-mirror"


def packages(system, box, repositories=None, groups=None):
    return {"system": system, "box": box, "owned": [], "groups": groups or {}, "repositories": repositories or {}}


class AtomicTest(HarnessTest):
    def dependencies(self):
        return self.harness.run(str(self.harness.source / "extras" / "packaging" / "dependencies.sh"))

    def inside(self):
        return [entry["argv"] for entry in self.harness.entries() if entry.get("inside")]


class TestFedoraAtomic(AtomicTest):
    system = "fedora-atomic"
    box = "konveyor-fedora-42"
    image = "registry.fedoraproject.org/fedora-toolbox:42"

    def test_a_new_toolbox_is_created_and_matched_to_the_system(self):
        self.harness.set_stub_state("podman", {"containers": {"konveyor-fedora-41": "old", "other": "x"}, "images": []})
        self.harness.set_stub_state("packages", packages({"kwin": "6.7.0-1.fc42", "qt6-qtbase": "6.9.1-2.fc42", "bash": "5.2-1"},
                                                         {"kwin": "6.7.0-1.fc42", "qt6-qtbase": "6.9.1-2.fc42"}))
        result = self.dependencies()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn(f"Creating the {self.box} toolbox", result.stdout)
        calls = self.harness.calls()
        self.assertIn(["toolbox", "rm", "--force", "konveyor-fedora-41"], calls)
        self.assertIn(["toolbox", "--assumeyes", "create", "--distro", "fedora", "--release", "42", self.box], calls)
        sudo = [call[1:] for call in calls if call[0] == "sudo"]
        self.assertEqual(sudo[:3], [["dnf", "install", "--assumeyes", "--quiet", "fedora-repos-archive"], ["dnf", "versionlock", "clear"],
                                    ["dnf", "versionlock", "add", "kwin-6.7.0-1.fc42", "qt6-qtbase-6.9.1-2.fc42"]])
        self.assertEqual(sudo[3][:3], ["dnf", "install", "--assumeyes"])
        self.assertEqual(sudo[4], ["dnf", "distro-sync", "--assumeyes"])
        self.assertEqual(self.harness.stub_state("podman")["containers"], {"other": "x", self.box: self.image})
        pulled = self.harness.home / ".local" / "share" / "konveyor" / "pulled-images"
        self.assertEqual(pulled.read_text(), f"{self.image}\n")

    def test_an_existing_toolbox_and_image_are_reused(self):
        self.harness.set_stub_state("podman", {"containers": {self.box: self.image}, "images": [self.image]})
        result = self.dependencies()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual([call for call in self.harness.calls("toolbox") if "create" in call], [])

    def test_uninstall_keeps_pulled_images_other_containers_use(self):
        self.harness.set_stub_state("podman", {"containers": {"mine": self.image}, "images": []})
        self.assertEqual(self.dependencies().returncode, 0)
        self.assertSucceeded(self.harness.functions("remove_unused_pulled_images"))
        pulled = self.harness.home / ".local" / "share" / "konveyor" / "pulled-images"
        self.assertEqual(pulled.read_text(), f"{self.image}\n")
        self.assertEqual(self.harness.stub_state("podman")["images"], [self.image])
        self.harness.set_stub_state("podman", {"containers": {}, "images": [self.image]})
        self.assertSucceeded(self.harness.functions("remove_unused_pulled_images"))
        self.assertFalse(pulled.exists())
        self.assertEqual(self.harness.stub_state("podman")["images"], [])

    def test_versions_the_repositories_lack_stop_the_install(self):
        self.harness.set_stub_state("packages", packages({"kwin": "6.7.0-1.fc42"}, {"kwin": "6.7.1-1.fc42"}))
        result = self.dependencies()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("kwin (system 6.7.0-1.fc42, toolbox 6.7.1-1.fc42)", result.stderr)

    def test_an_image_based_system_that_is_not_fedora_is_refused(self):
        self.harness.write(self.harness.system / "etc" / "os-release", "ID=centos\nVERSION_ID=10\n")
        result = self.dependencies()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("isn't based on Fedora", result.stderr)
        self.harness.write(self.harness.system / "etc" / "os-release", "ID=aurora\nID_LIKE=\"ublue fedora\"\nVERSION_ID=42\n")
        self.assertEqual(self.dependencies().returncode, 0)

    def test_a_missing_toolbox_is_reported(self):
        (self.harness.stubs / "toolbox").unlink()
        result = self.dependencies()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("toolbox is missing", result.stderr)

    def test_the_build_runs_in_the_toolbox_and_installs_into_the_home(self):
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        builds = [call for call in self.inside() if call[0] == "cmake"]
        self.assertEqual([call[1] for call in builds], ["-S", "--build", "--install"])
        self.assertIn(f"-DCMAKE_INSTALL_PREFIX={self.harness.home}/.local", builds[0])
        plugins = self.harness.home / ".local" / "lib" / "plugins" / "kwin" / "effects" / "plugins"
        self.assertEqual(sorted(path.name for path in plugins.iterdir()), ["konveyor_effect_1700000000.so"])
        fingerprint = (self.harness.home / ".local" / "share" / "konveyor" / "built-for").read_text()
        self.assertIn("image=42.20250101.0\n", fingerprint)

    def test_reclaim_covers_the_state_and_every_installed_file(self):
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertSucceeded(self.harness.functions("reclaim_files", env={"STUB_UID": "4242"}))
        chown = self.harness.calls("sudo")[0]
        state = self.harness.home / ".local" / "share" / "konveyor"
        self.assertIn(str(state), chown)
        self.assertIn(str(self.harness.home / ".local" / "bin" / "konveyor"), chown)


class TestSessionPaths(AtomicTest):
    system = "fedora-atomic"

    def environment(self):
        return self.harness.stub_state("systemd")["environment"]

    def test_the_session_finds_the_plugins_and_qml_in_the_home(self):
        self.harness.set_stub_state("systemd", {"environment": {"QT_PLUGIN_PATH": "/usr/lib/qt6/plugins"}, "enabled": []})
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        local = self.harness.home / ".local"
        expected = {"QT_PLUGIN_PATH": f"{local}/lib/plugins:/usr/lib/qt6/plugins", "QML_IMPORT_PATH": f"{local}/lib/qml"}
        self.assertEqual(self.environment(), expected)
        conf = (self.harness.home / ".config" / "environment.d" / "konveyor.conf").read_text()
        self.assertEqual(conf, f"QT_PLUGIN_PATH={local}/lib/plugins${{QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}}\n"
                               f"QML_IMPORT_PATH={local}/lib/qml${{QML_IMPORT_PATH:+:$QML_IMPORT_PATH}}\n")
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertEqual(self.environment(), expected)
        self.assertSucceeded(self.harness.uninstall())
        self.assertEqual(self.environment(), {"QT_PLUGIN_PATH": "/usr/lib/qt6/plugins"})
        self.assertFalse((self.harness.home / ".config" / "environment.d").exists())

    def test_updates_run_at_login_from_the_home(self):
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        updater = self.harness.home / ".local" / "lib" / "konveyor"
        self.assertEqual(sorted(path.name for path in updater.iterdir()), ["common.sh", "install", "install-prefix", "konveyor-rebuild", "updates.sh"])
        self.assertEqual((updater / "install-prefix").read_text(), f"{self.harness.home}/.local\n")
        unit = (self.harness.home / ".config" / "systemd" / "user" / "konveyor-login-update.service").read_text()
        self.assertIn(f"Environment=KONVEYOR_SOURCE_DIR={self.harness.home}/.local/share/konveyor/source\n", unit)
        self.assertEqual(self.harness.stub_state("systemd")["enabled"], ["konveyor-login-update.service"])
        self.assertEqual(self.harness.calls("sudo"), [])


class TestSteamOS(AtomicTest):
    system = "steamos"
    box = "konveyor-steamos"

    def test_the_box_is_created_and_pinned_to_the_system_versions(self):
        self.harness.set_stub_state("packages", packages({"kwin": "6.7.0-1", "kf6-kconfig": "6.19-1", "qt6-base": "6.9.1-1"},
                                                         {"kwin": "6.7.2-1", "kf6-kconfig": "6.19-1", "qt6-base": "6.9.1-1"},
                                                         {"kwin": "jupiter-3.9"}, {"kf6": ["kf6-kconfig"]}))
        result = self.dependencies()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        entries = self.harness.entries()
        calls = [entry["argv"] for entry in entries]
        self.assertIn(["distrobox", "create", "--yes", "--no-entry", "--name", self.box, "--image", "docker.io/library/archlinux:latest"], calls)
        confs = {entry["argv"][2]: entry["stdin"] for entry in entries if entry["argv"][:2] == ["sudo", "tee"]}
        repositories = f"[jupiter-3.9]\nServer = {MIRROR}/$repo/os/$arch\n[holo-3.9]\nServer = {MIRROR}/$repo/os/$arch\n"
        self.assertEqual(confs["/etc/pacman.conf"], "[options]\nArchitecture = auto\nSigLevel = Required DatabaseOptional\n"
                                                    f"LocalFileSigLevel = Optional\n{repositories}")
        self.assertIn("SigLevel = Never\n", confs["/etc/pacman-keyring.conf"])
        self.assertIn(["sudo", "pacman", "-U", "--noconfirm", f"{MIRROR}/jupiter-3.9/os/x86_64/kwin-6.7.0-1-x86_64.pkg.tar.zst"], calls)
        self.assertTrue(all(entry.get("inside") == self.box for entry in entries if entry["argv"][0] == "sudo"))

    def test_versions_valve_lacks_stop_the_install(self):
        self.harness.set_stub_state("packages", packages({"kwin": "6.7.0-1"}, {"kwin": "6.7.2-1"}))
        result = self.dependencies()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("kwin (system 6.7.0-1, box 6.7.2-1)", result.stderr)

    def test_an_older_steamos_is_sent_to_the_preview_channel(self):
        for name in ("libkwin.so.6",):
            (self.harness.system / "usr" / "lib" / name).unlink()
            (self.harness.system / "usr" / "lib" / name).symlink_to("libkwin.so.6.4.0")
        result = self.dependencies()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("this SteamOS build has KWin 6.4.0", result.stderr)
        self.assertEqual(self.harness.calls("distrobox"), [])

    def test_a_missing_distrobox_is_reported(self):
        (self.harness.stubs / "distrobox").unlink()
        result = self.dependencies()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("distrobox is missing", result.stderr)

    def test_the_build_runs_in_the_box(self):
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertEqual([call[1] for call in self.inside() if call[0] == "cmake"], ["-S", "--build", "--install"])
        self.assertIn("image=20250101.1", (self.harness.home / ".local" / "share" / "konveyor" / "built-for").read_text())


if __name__ == "__main__":
    unittest.main()
