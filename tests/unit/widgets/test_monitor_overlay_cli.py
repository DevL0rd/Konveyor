#!/usr/bin/env python3
import json
import os
import unittest
from unittest import mock

from portal_harness import REPO, load_script
from router_harness import RouterTest


SCRIPT = REPO / "widgets" / "service" / "monitor-overlay"
USAGE = "usage: monitor-overlay [toggle|show|hide|remove <window-id>|update-fullscreen <window-id> <0|1>]\n"
KONVEYOR = """entry = {"tool": "konveyor", "argv": sys.argv[1:]}
open(os.environ["STUB_LOG"] + ".tools", "a").write(json.dumps(entry) + "\\n")
reply = os.path.join(os.environ["KONVEYOR_REPLIES"], sys.argv[-1])
if not os.path.exists(reply):
    sys.exit(1)
sys.stdout.write(open(reply).read())
"""
WINDOW = {"app_id": "steam_app_1", "id": 11, "pid": 900, "layout": {"tile_pos_in_workspace_view": [1920, 1081],
                                                                    "window_size": [2560, 1440]}}
OUTPUT = [{"name": "DP-2", "logical": {"x": 1920, "y": 1080, "width": 2560, "height": 1440}}]


class TestMonitorOverlayCli(RouterTest):
    stubs = ()

    def setUp(self):
        super().setUp()
        self.replies = self.box.root / "konveyor"
        self.replies.mkdir()
        self.box.environment["KONVEYOR_REPLIES"] = str(self.replies)
        stub = self.script_stub("konveyor", KONVEYOR)
        local = self.box.home / ".local" / "bin"
        local.mkdir(parents=True)
        stub.rename(local / "konveyor")
        self.state = self.box.runtime / "Konveyor-Monitor-Overlay" / "state.json"

    def reply(self, request, value):
        (self.replies / request).write_text(value if isinstance(value, str) else json.dumps(value))

    def overlay(self, *arguments, code=0):
        result = self.invoke(SCRIPT, *map(str, arguments))
        self.assertEqual(result.returncode, code, result.stderr)
        self.assertNotIn("Traceback", result.stderr)
        return result

    def targets(self):
        return json.loads(self.state.read_text())["targets"]

    def test_finds_konveyor_in_local_bin(self):
        self.reply("focused-window", WINDOW)
        self.reply("focused-output", OUTPUT)
        self.overlay("show")
        self.assertEqual(self.targets(), [{"key": "steam_app_1", "appId": "steam_app_1", "pid": 900, "windowId": 11, "x": 1920,
                                           "y": 1080, "width": 2560, "height": 1440, "fullscreen": True}])
        self.assertEqual([call["argv"] for call in self.tool_calls("konveyor")],
                         [["msg", "--json", "focused-window"], ["msg", "--json", "focused-output"]])

    def test_toggle_is_the_default(self):
        self.reply("focused-window", WINDOW)
        self.overlay()
        self.assertEqual(self.targets()[0]["fullscreen"], False)
        self.overlay()
        self.assertEqual(self.targets(), [])

    def test_usage_errors(self):
        for arguments in (("dance",), ("remove",), ("update-fullscreen", 11), ("remove", "eleven"),
                          ("update-fullscreen", "x", 1), ("update-fullscreen", 11, "yes")):
            self.assertEqual(self.overlay(*arguments, code=2).stderr, USAGE, arguments)
        self.assertFalse(self.state.exists())
        self.assertEqual(self.tool_calls(), [])

    def test_nothing_focused_leaves_the_state_alone(self):
        self.overlay("show", code=1)
        self.assertFalse(self.state.exists())
        for window in ("not json", [WINDOW], {**WINDOW, "app_id": ""}, {**WINDOW, "layout": {}},
                       {**WINDOW, "layout": {"tile_pos_in_workspace_view": [0], "window_size": [10, 10]}},
                       {**WINDOW, "layout": {"tile_pos_in_workspace_view": [0, 0], "window_size": [0, 10]}}):
            self.reply("focused-window", window)
            self.overlay("toggle", code=1)
        self.assertFalse(self.state.exists())

    def test_without_an_output_nothing_is_fullscreen(self):
        self.reply("focused-window", WINDOW)
        for output in ("[]", "{}", "oops"):
            self.reply("focused-output", output)
            self.overlay("show")
            self.assertEqual((self.targets()[0]["fullscreen"], self.targets()[0]["y"]), (False, 1081))

    def test_several_applications(self):
        self.reply("focused-window", WINDOW)
        self.overlay("show")
        self.reply("focused-window", {**WINDOW, "app_id": "firefox", "id": 12})
        self.overlay("show")
        self.assertEqual([target["key"] for target in self.targets()], ["steam_app_1", "firefox"])
        self.overlay("toggle")
        self.assertEqual([target["key"] for target in self.targets()], ["steam_app_1"])
        self.overlay("remove", 99)
        self.assertEqual(len(self.targets()), 1)
        self.overlay("hide")
        self.assertEqual(self.targets(), [])

    def test_unchanged_fullscreen_is_not_rewritten(self):
        self.reply("focused-window", WINDOW)
        self.overlay("show")
        before = self.state.read_text()
        self.overlay("update-fullscreen", 11, 0)
        self.overlay("update-fullscreen", 42, 1)
        self.assertEqual(self.state.read_text(), before)
        self.overlay("update-fullscreen", 11, 1)
        self.assertTrue(self.targets()[0]["fullscreen"])
        self.assertNotEqual(json.loads(self.state.read_text())["generation"], json.loads(before)["generation"])

    def test_a_damaged_state_starts_over(self):
        self.reply("focused-window", WINDOW)
        for state in ("{broken", '{"targets": {"a": 1}}', "[]"):
            self.box.write(self.state, state)
            self.overlay("show")
            self.assertEqual([target["key"] for target in self.targets()], ["steam_app_1"])

    def test_default_runtime_directory(self):
        module = load_script(SCRIPT, self.box.environment)
        with mock.patch.dict(os.environ, {}, clear=True):
            self.assertEqual(str(module.runtime_directory()), f"/run/user/{os.getuid()}/Konveyor-Monitor-Overlay")
        self.assertEqual(sorted(path.name for path in self.box.runtime.iterdir()), [])


if __name__ == "__main__":
    unittest.main()
