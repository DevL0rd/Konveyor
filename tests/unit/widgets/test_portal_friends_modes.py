#!/usr/bin/env python3
import importlib.machinery
import importlib.util
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[3] / "widgets" / "portals" / "bin" / "portal-friends"


class StopServing(Exception):
    pass


class TestPortalFriendsModes(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        root = Path(self.temporary.name)
        self.environment = {
            "PATH": os.environ.get("PATH", ""),
            "HOME": str(root / "home"),
            "XDG_CONFIG_HOME": str(root / "config"),
            "XDG_CACHE_HOME": str(root / "cache"),
            "XDG_RUNTIME_DIR": str(root / "runtime"),
        }
        self.config = root / "config" / "Plasma-App-Portal" / "config.json"
        self.snapshot = root / "runtime" / "Plasma-App-Portal" / "friends.json"

    def tearDown(self):
        self.temporary.cleanup()

    def configure(self, text):
        self.config.parent.mkdir(parents=True, exist_ok=True)
        self.config.write_text(text)

    def run_script(self, *arguments):
        return subprocess.run([sys.executable, str(SCRIPT), *arguments], env=self.environment, capture_output=True, text=True, timeout=20)

    def load(self):
        with mock.patch.dict(os.environ, self.environment, clear=True):
            loader = importlib.machinery.SourceFileLoader("portal_friends_modes", str(SCRIPT))
            spec = importlib.util.spec_from_loader(loader.name, loader)
            module = importlib.util.module_from_spec(spec)
            loader.exec_module(module)
        return module

    def test_an_unknown_mode_prints_usage_instead_of_polling(self):
        result = self.run_script("--chek")
        self.assertEqual(result.returncode, 2)
        self.assertEqual(result.stdout, "")
        self.assertIn("unknown mode '--chek'", result.stderr)
        for mode in ("--check", "--snapshot", "--serve", "--set-key"):
            self.assertIn(mode, result.stderr)

    def test_no_mode_still_checks(self):
        result = self.run_script()
        self.assertEqual(result.returncode, 1)
        self.assertIn("NOT OK: no steam_api_key in config", result.stdout)

    def test_serve_rejects_a_poll_interval_that_is_not_a_number(self):
        for value in ('"fast"', "null", "[10]", "NaN", "Infinity"):
            with self.subTest(value=value):
                self.configure('{"poll_interval": %s}' % value)
                result = self.run_script("--serve")
                self.assertEqual(result.returncode, 1)
                self.assertIn("poll_interval must be", result.stderr)
                self.assertNotIn("Traceback", result.stderr)
                snapshot = json.loads(self.snapshot.read_text())
                self.assertFalse(snapshot["ok"])
                self.assertIn("poll_interval", snapshot["error"])

    def test_serve_refuses_to_poll_in_a_tight_loop(self):
        for value in ("0", "-5", "4.9", "true"):
            with self.subTest(value=value):
                self.configure('{"poll_interval": %s}' % value)
                result = self.run_script("--serve")
                self.assertEqual(result.returncode, 1)
                self.assertIn("poll_interval must be at least 5 seconds", result.stderr)

    def test_serve_sleeps_the_configured_interval_between_polls(self):
        for text, expected in (('{"poll_interval": 5}', 5.0), ('{"poll_interval": "12.5"}', 12.5), ("{}", 60.0), ("not json", 60.0)):
            with self.subTest(text=text):
                self.configure(text)
                module = self.load()
                with mock.patch.object(module.time, "sleep", side_effect=StopServing) as sleep, mock.patch.object(module, "api") as api:
                    with self.assertRaises(StopServing):
                        module.cmd_serve()
                sleep.assert_called_once_with(expected)
                api.assert_not_called()
                self.assertTrue(json.loads(self.snapshot.read_text())["needs_api_key"])

    def test_serve_picks_up_a_changed_interval(self):
        self.configure('{"poll_interval": 30}')
        module = self.load()
        intervals = []

        def sleep(seconds):
            intervals.append(seconds)
            if len(intervals) == 2:
                raise StopServing
            self.configure('{"poll_interval": 45}')

        with mock.patch.object(module.time, "sleep", side_effect=sleep):
            with self.assertRaises(StopServing):
                module.cmd_serve()
        self.assertEqual(intervals, [30.0, 45.0])


if __name__ == "__main__":
    unittest.main()
