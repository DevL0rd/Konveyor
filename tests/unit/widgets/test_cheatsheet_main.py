#!/usr/bin/env python3
import io
import json
import os
import signal
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from cheatsheet_module import SCRIPT, load_module


RECORDER = """#!/usr/bin/env python3
import json, os, sys
with open(os.environ["STUB_LOG"], "a") as log:
    log.write(json.dumps({"argv": sys.argv[1:], "shell": os.environ.get("QT_WAYLAND_SHELL_INTEGRATION")}) + "\\n")
sys.exit(int(os.environ.get("STUB_EXIT", "0")))
"""
SECTIONS = [{"name": "Focus", "entries": [{"action": "Focus column left", "keys": ["Mod+H"], "id": "focus-column-left"}]}]


class TestCheatsheetMain(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.module, self.environment = load_module(self.root)
        self.log = self.root / "calls"
        self.viewer = self.root / "viewer"
        self.launcher = self.root / "home" / ".local" / "bin" / "portal-launcher"
        self.patches = [mock.patch.dict(os.environ, {**self.environment, "STUB_LOG": str(self.log)}),
                        mock.patch.object(self.module, "build_sections", lambda: SECTIONS)]
        for patch in self.patches:
            patch.start()

    def tearDown(self):
        for patch in reversed(self.patches):
            patch.stop()
        self.temporary.cleanup()

    def stub(self, path, exit_code=0):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(RECORDER.replace('"0"', f'"{exit_code}"'))
        path.chmod(0o755)

    def sleeper(self, *arguments):
        process = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)", *arguments])
        self.addCleanup(process.wait)
        self.addCleanup(process.kill)
        return process

    def calls(self):
        return [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []

    def main(self, *arguments):
        stderr = io.StringIO()
        stdout = io.StringIO()
        with mock.patch.object(sys, "argv", ["konveyor-cheatsheet", *arguments]), mock.patch.object(sys, "stderr", stderr), \
                mock.patch.object(sys, "stdout", stdout), mock.patch.object(self.module, "notify") as notify:
            code = self.module.main()
        return code, stdout.getvalue(), stderr.getvalue(), notify

    def run_script(self, *arguments):
        return subprocess.run([sys.executable, str(SCRIPT), *arguments], env={**os.environ, "PATH": os.environ["PATH"]},
                              capture_output=True, text=True, timeout=30)

    def test_json_prints_the_sections(self):
        code, stdout, _, _ = self.main("--json")
        self.assertEqual((code, json.loads(stdout)), (0, SECTIONS))

    def test_other_arguments_print_the_usage(self):
        code, _, stderr, _ = self.main("--help")
        self.assertEqual(code, 2)
        self.assertIn("usage: konveyor-cheatsheet [--json]", stderr)
        self.assertEqual(self.calls(), [])

    def test_json_without_the_session_bus_fails_with_one_clear_line(self):
        result = self.run_script("--json")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "")
        self.assertNotIn("Traceback", result.stderr)
        self.assertEqual(len(result.stderr.strip().splitlines()), 1)
        self.assertRegex(result.stderr, r"^konveyor-cheatsheet: could not reach Konveyor on the session bus")

    def test_the_viewer_is_not_started_without_the_session_bus(self):
        self.stub(self.viewer)
        result = self.run_script()
        self.assertEqual(result.returncode, 1)
        self.assertNotIn("Traceback", result.stderr)
        self.assertIn("could not reach Konveyor", result.stderr)
        self.assertIn("could not show a notification", result.stderr)
        self.assertEqual(self.calls(), [])

    def test_the_viewer_path_notifies_when_konveyor_is_unreachable(self):
        self.patches[1].stop()
        self.patches.pop()
        code, _, stderr, notify = self.main()
        self.assertEqual(code, 1)
        self.assertIn("could not reach Konveyor", stderr)
        notify.assert_called_once()
        self.assertIn("could not reach Konveyor", notify.call_args.args[1])

    def test_the_portal_launcher_opens_its_shortcuts_page(self):
        self.stub(self.launcher)
        self.stub(self.viewer)
        self.assertEqual(self.main()[0], 0)
        self.assertEqual([call["argv"] for call in self.calls()], [["shortcuts"]])

    def test_a_failing_portal_launcher_passes_its_exit_code_on(self):
        self.stub(self.launcher, exit_code=3)
        self.stub(self.viewer)
        code, _, _, notify = self.main()
        self.assertEqual(code, 3)
        self.assertEqual([call["argv"] for call in self.calls()], [["shortcuts"]])
        notify.assert_not_called()

    def test_a_portal_launcher_that_cannot_run_fails_visibly(self):
        self.launcher.parent.mkdir(parents=True)
        self.launcher.write_text("#!/bin/sh\n")
        self.stub(self.viewer)
        code, _, stderr, notify = self.main()
        self.assertEqual(code, 1)
        self.assertIn(f"could not start {self.launcher}", stderr)
        notify.assert_called_once()
        self.assertEqual(self.calls(), [])

    def test_starts_the_viewer_with_the_sections_and_cleans_its_pid_file(self):
        self.stub(self.viewer)
        self.assertEqual(self.main()[0], 0)
        self.assertEqual(self.calls(), [{"argv": [str(self.module.QML), json.dumps(SECTIONS)], "shell": "layer-shell"}])
        self.assertFalse(self.module.PID_FILE.exists())

    def test_passes_the_viewer_exit_code_on(self):
        self.stub(self.viewer, exit_code=4)
        self.assertEqual(self.main()[0], 4)

    def test_a_missing_viewer_fails_visibly(self):
        code, _, stderr, notify = self.main()
        self.assertEqual(code, 1)
        self.assertIn(f"could not start {self.viewer}", stderr)
        notify.assert_called_once()

    def test_a_second_run_closes_the_open_viewer(self):
        viewer = self.sleeper(str(self.module.QML))
        self.module.PID_FILE.write_text(str(viewer.pid))
        self.stub(self.viewer)
        self.assertEqual(self.main()[0], 0)
        self.assertEqual(viewer.wait(timeout=10), -signal.SIGTERM)
        self.assertFalse(self.module.PID_FILE.exists())
        self.assertEqual(self.calls(), [])

    def test_a_pid_of_another_program_is_left_alone(self):
        other = self.sleeper()
        self.module.PID_FILE.write_text(str(other.pid))
        self.assertFalse(self.module.close_running())
        self.assertIsNone(other.poll())
        self.assertFalse(self.module.PID_FILE.exists())

    def test_a_stale_or_broken_pid_file_is_removed(self):
        finished = subprocess.run([sys.executable, "-c", "import os; print(os.getpid())"], capture_output=True, text=True)
        for contents in (finished.stdout.strip(), "not a pid", ""):
            self.module.PID_FILE.write_text(contents)
            self.assertFalse(self.module.close_running())
            self.assertFalse(self.module.PID_FILE.exists())
        self.assertFalse(self.module.close_running())

    def test_a_viewer_that_exits_before_the_signal_is_not_an_error(self):
        viewer = self.sleeper(str(self.module.QML))
        self.module.PID_FILE.write_text(str(viewer.pid))
        with mock.patch.object(self.module.os, "kill", side_effect=ProcessLookupError):
            self.assertFalse(self.module.close_running())
        self.assertFalse(self.module.PID_FILE.exists())

    def test_the_viewer_keeps_a_newer_pid_file(self):
        self.viewer.write_text(f"#!/bin/sh\necho 999999 > {self.module.PID_FILE}\n")
        self.viewer.chmod(0o755)
        self.assertEqual(self.main()[0], 0)
        self.assertEqual(self.module.PID_FILE.read_text().strip(), "999999")

    def test_notify_reports_an_unreachable_bus(self):
        stderr = io.StringIO()
        with mock.patch.object(sys, "stderr", stderr):
            self.module.notify("Summary", "Message")
        self.assertIn("konveyor-cheatsheet: could not show a notification", stderr.getvalue())


if __name__ == "__main__":
    unittest.main()
