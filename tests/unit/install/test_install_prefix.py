#!/usr/bin/env python3
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import HarnessTest

SCRIPTS = ("karousel", "krohnkite", "kzones", "polonium", "bismuth", "devl0rd-hide-desktop-widgets")


class TestPrefixFiles(HarnessTest):
    def build_dir(self):
        return self.harness.source / "build-release"

    def test_reclaim_takes_back_files_another_user_owns(self):
        self.build_dir().mkdir()
        result = self.assertSucceeded(self.harness.functions("reclaim_files", env={"STUB_UID": "4242"}))
        self.assertIn("Taking back files an earlier install left owned by another user", result.stdout)
        chown = self.harness.calls("sudo")
        self.assertEqual(len(chown), 1)
        self.assertEqual(chown[0][:3], ["sudo", "chown", "-R"])
        self.assertEqual(chown[0][4:], [str(self.build_dir())])

    def test_reclaim_leaves_owned_or_missing_files_alone(self):
        self.assertSucceeded(self.harness.functions("reclaim_files", env={"STUB_UID": "4242"}))
        self.build_dir().mkdir()
        self.assertSucceeded(self.harness.functions("reclaim_files"))
        self.assertEqual(self.harness.calls("sudo"), [])

    def test_stale_files_inside_the_prefix_are_removed(self):
        prefix = self.harness.prefix
        kept, stale, missing = prefix / "bin" / "konveyor", prefix / "lib" / "old.so", prefix / "lib" / "gone.so"
        for path in (kept, stale):
            self.harness.write(path, "")
        outside = self.harness.root / "outside"
        self.harness.write(outside, "")
        previous = [kept, stale, missing, outside, prefix / "lib" / ".." / "lib" / "old.so"]
        self.harness.write(prefix / "share" / "konveyor" / "install_manifest.txt", "\n".join(map(str, previous)))
        self.harness.write(self.build_dir() / "install_manifest.txt", str(kept))
        self.assertSucceeded(self.harness.functions("remove_stale_files"))
        self.assertEqual(self.harness.calls("sudo"), [["sudo", "rm", "-f", str(stale)]])

    def test_without_an_earlier_manifest_nothing_is_stale(self):
        self.assertSucceeded(self.harness.functions("remove_stale_files"))
        self.assertEqual(self.harness.calls(), [])

    def test_install_files_records_the_manifest_and_finds_the_plugin_directory(self):
        plugins = self.harness.seed_prefix()
        result = self.assertSucceeded(self.harness.functions('install_files; printf "%s\\n" "$KONVEYOR_PLUGIN_DIR"'))
        self.assertEqual(result.stdout.splitlines()[-1], str(plugins))
        calls = self.harness.calls("sudo")
        self.assertEqual(calls[0], ["sudo", "cmake", "--install", str(self.build_dir())])
        self.assertIn(["sudo", "install", "-Dm644", str(self.build_dir() / "install_manifest.txt"),
                       str(self.harness.prefix / "share" / "konveyor" / "install_manifest.txt")], calls)

    def test_a_manifest_without_the_effect_fails_visibly(self):
        self.harness.write(self.harness.prefix / "share" / "konveyor" / "install_manifest.txt", "/nothing\n")
        result = self.harness.functions("konveyor_plugin_dir")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("lists no file matching", result.stderr)

    def test_each_install_gets_a_new_plugin_id_and_drops_the_old_ones(self):
        plugins = self.harness.seed_prefix()
        stray = self.harness.prefix / "lib" / "qt6" / "plugins" / "kwin" / "effects" / "plugins"
        for name in ("konveyor_effect_7.so", "process_monitor_telemetry_7.so", "unrelated.so"):
            self.harness.write(stray / name, "")
        self.harness.write(self.harness.home / ".config" / "kwinrc",
                           "[Plugins]\nkonveyor_effect_1Enabled=true\nkonveyor_effect_0Enabled=true\nprocess_monitor_telemetry_1Enabled=true\n")
        self.harness.set_stub_state("kwin", {"running": True, "loaded": ["konveyor_effect_1", "konveyor_effect_x"], "refuse": []})
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets", env={"STUB_DATE": "2"}))
        self.assertEqual(self.harness.config("kwinrc")[("Plugins",)], {"konveyor_effectEnabled": "false", "konveyor_effect_2Enabled": "true"})
        self.assertEqual(self.harness.stub_state("kwin")["loaded"], ["konveyor_effect_2"])
        removed = {call[3] for call in self.harness.calls("sudo") if call[1:3] == ["rm", "-f"]}
        for name in ("konveyor_effect_1.so", "process_monitor_telemetry_1.so", "konveyor_effect.so", "process_monitor_telemetry.so"):
            self.assertIn(str(plugins / name), removed)
        self.assertIn(str(stray / "konveyor_effect_7.so"), removed)
        self.assertIn(str(stray / "process_monitor_telemetry_7.so"), removed)
        self.assertNotIn(str(stray / "unrelated.so"), removed)

    def test_the_new_plugin_ids_are_recorded(self):
        self.harness.seed_prefix()
        self.assertSucceeded(self.harness.install("--skip-deps", env={"STUB_DATE": "5"}))
        tees = {call[2]: entry.get("stdin") for entry in self.harness.entries() for call in [entry["argv"]] if call[:2] == ["sudo", "tee"]}
        state = self.harness.prefix / "share" / "konveyor"
        self.assertEqual(tees[str(state / "plugin-id")], "konveyor_effect_5\n")
        self.assertEqual(tees[str(state / "telemetry-plugin-id")], "process_monitor_telemetry_5\n")
        self.assertIn("kwin=6.7.0\n", tees[str(state / "built-for")])


class TestConflictingScripts(HarnessTest):
    def setUp(self):
        super().setUp()
        home = self.harness.home
        self.harness.write(home / ".local" / "share" / "kwin" / "scripts" / "karousel" / "metadata.json", "{}")
        self.harness.write(self.harness.system / "usr" / "share" / "kwin-wayland" / "scripts" / "krohnkite" / "metadata.json", "{}")
        self.harness.write(self.harness.system / "usr" / "local" / "share" / "kwin" / "scripts" / "polonium" / "metadata.json", "{}")
        self.harness.write(home / ".config" / "kwinrc", "[Plugins]\nkarouselEnabled=true\npoloniumEnabled=false\nkzonesEnabled=true\n")
        self.disabled = home / ".local" / "state" / "konveyor" / "disabled-scripts"

    def plugins(self):
        return self.harness.config("kwinrc").get(("Plugins",), {})

    def test_install_turns_off_every_installed_tiling_script_and_remembers_how_it_was(self):
        self.assertSucceeded(self.harness.functions("disable_conflicting_scripts"))
        self.assertEqual(self.plugins(), {"karouselEnabled": "false", "poloniumEnabled": "false", "krohnkiteEnabled": "false"})
        self.assertEqual(self.disabled.read_text(), "karousel true\nkrohnkite \npolonium false\n")
        self.assertSucceeded(self.harness.functions("disable_conflicting_scripts"))
        self.assertEqual(self.disabled.read_text(), "karousel true\nkrohnkite \npolonium false\n")

    def test_restore_puts_every_script_back(self):
        self.assertSucceeded(self.harness.functions("disable_conflicting_scripts"))
        self.assertSucceeded(self.harness.functions("restore_conflicting_scripts"))
        self.assertEqual(self.plugins(), {"karouselEnabled": "true", "poloniumEnabled": "false"})
        self.assertFalse(self.disabled.exists())

    def test_a_script_removed_while_installed_loses_its_setting(self):
        self.assertSucceeded(self.harness.functions("disable_conflicting_scripts"))
        for path in (self.harness.home / ".local" / "share" / "kwin" / "scripts" / "karousel" / "metadata.json",):
            path.unlink()
            path.parent.rmdir()
        self.assertSucceeded(self.harness.functions("disable_conflicting_scripts"))
        self.assertEqual(self.plugins()["karouselEnabled"], "false")
        self.assertSucceeded(self.harness.functions("restore_conflicting_scripts"))
        self.assertNotIn("karouselEnabled", self.plugins())

    def test_install_and_uninstall_round_trip_the_scripts(self):
        before = self.plugins()
        self.harness.seed_prefix()
        self.assertSucceeded(self.harness.install("--skip-deps", "--no-widgets"))
        self.assertEqual(self.plugins()["karouselEnabled"], "false")
        self.assertSucceeded(self.harness.uninstall())
        before.pop("kzonesEnabled")
        self.assertEqual(self.plugins(), before)


if __name__ == "__main__":
    unittest.main()
