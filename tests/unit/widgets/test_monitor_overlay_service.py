#!/usr/bin/env python3
import importlib.machinery
import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


REPO = Path(__file__).resolve().parents[3]
SCRIPT = REPO / "widgets" / "service" / "monitor-overlay"


def load_module():
    loader = importlib.machinery.SourceFileLoader("monitor_overlay_service", str(SCRIPT))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    return module


class TestMonitorOverlayService(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.directory = Path(self.temporary.name)
        self.module = load_module()
        self.window = {
            "app_id": "google-chrome",
            "id": 7,
            "pid": 1234,
            "layout": {
                "tile_pos_in_workspace_view": [100, 40],
                "window_size": [1200, 800],
            },
        }
        self.output = {"logical": {"x": 0, "y": 0, "width": 1920, "height": 1080}}

    def tearDown(self):
        self.temporary.cleanup()

    def run_mode(self, mode, *arguments):
        def response(request):
            if request == "focused-window":
                return self.window
            if request == "focused-output":
                return [self.output]
            return None

        argv = [str(SCRIPT), mode, *map(str, arguments)]
        with patch.object(self.module, "runtime_directory", return_value=self.directory), patch.object(
            self.module, "konveyor_json", side_effect=response
        ), patch.object(sys, "argv", argv):
            self.assertEqual(self.module.main(), 0)
        return json.loads((self.directory / "state.json").read_text())

    def test_konveyor_is_found_in_local_bin_first(self):
        with patch.object(self.module.subprocess, "run") as run, patch.dict(self.module.os.environ, {"PATH": "/usr/bin"}):
            run.return_value.stdout = "[]"
            self.module.konveyor_json("focused-output")
        self.assertEqual(run.call_args.args[0][0], "konveyor")
        self.assertEqual(run.call_args.kwargs["env"]["PATH"], f"{Path.home() / '.local' / 'bin'}:/usr/bin")

    def test_toggle_is_per_application(self):
        state = self.run_mode("toggle")
        self.assertEqual(len(state["targets"]), 1)
        self.assertEqual(state["targets"][0]["key"], "google-chrome")
        state = self.run_mode("toggle")
        self.assertEqual(state["targets"], [])

    def test_show_replaces_existing_application_window(self):
        self.run_mode("show")
        self.window["id"] = 9
        self.window["pid"] = 4321
        self.window["layout"]["tile_pos_in_workspace_view"] = [220, 70]
        state = self.run_mode("show")
        self.assertEqual(len(state["targets"]), 1)
        target = state["targets"][0]
        self.assertEqual(target["windowId"], 9)
        self.assertEqual(target["pid"], 4321)
        self.assertEqual(target["x"], 220)

    def test_fullscreen_uses_output_geometry(self):
        self.window["layout"]["tile_pos_in_workspace_view"] = [0, 0]
        self.window["layout"]["window_size"] = [1920, 1080]
        state = self.run_mode("show")
        target = state["targets"][0]
        self.assertTrue(target["fullscreen"])
        self.assertEqual(target["y"], 0)

    def test_fullscreen_update_remove_and_hide(self):
        state = self.run_mode("show")
        window_id = state["targets"][0]["windowId"]
        state = self.run_mode("update-fullscreen", window_id, 1)
        self.assertTrue(state["targets"][0]["fullscreen"])
        state = self.run_mode("remove", window_id)
        self.assertEqual(state["targets"], [])
        self.run_mode("show")
        state = self.run_mode("hide")
        self.assertEqual(state["targets"], [])


if __name__ == "__main__":
    unittest.main()
