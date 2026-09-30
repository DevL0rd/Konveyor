#!/usr/bin/env python3
import json
import subprocess
import unittest

from portal_harness import REPO
from router_harness import RouterTest


SCRIPT = REPO / "widgets" / "screen-rotate" / "bin" / "linux-plasma-screen-rotate"
KSCREEN_DOCTOR = """entry = {"tool": "kscreen-doctor", "argv": sys.argv[1:]}
open(os.environ["STUB_LOG"] + ".tools", "a").write(json.dumps(entry) + "\\n")
if sys.argv[1:] == ["-j"]:
    sys.stdout.write(os.environ.get("SCREENS", ""))
    sys.exit(int(os.environ.get("READ_CODE", "0")))
sys.exit(int(os.environ.get("ROTATE_CODE", "0")))
"""


def output(name, rotation, enabled=True):
    return {"id": len(name), "name": name, "enabled": enabled, "connected": True, "rotation": rotation}


class TestScreenRotate(RouterTest):
    stubs = ()
    linked = ("bash", "jq")

    def setUp(self):
        super().setUp()
        self.script_stub("kscreen-doctor", KSCREEN_DOCTOR)

    def screens(self, *outputs):
        self.box.environment["SCREENS"] = json.dumps({"outputs": list(outputs), "screen": {"id": 0}})

    def rotate(self, *arguments, code=0):
        result = subprocess.run([str(SCRIPT), *arguments], env=self.box.environment, capture_output=True, text=True,
                                stdin=subprocess.DEVNULL)
        self.assertEqual(result.returncode, code, result.stdout + result.stderr)
        return result

    def rotations(self):
        return [call["argv"][0] for call in self.tool_calls("kscreen-doctor") if call["argv"] != ["-j"]]

    def test_state_is_the_default(self):
        self.screens(output("DP-1", 1))
        self.assertEqual(self.rotate().stdout, "landscape\n")
        self.assertEqual(self.rotations(), [])

    def test_state_follows_the_first_enabled_output(self):
        for rotation, state in ((1, "landscape"), (2, "portrait"), (4, "landscape"), (8, "portrait")):
            self.screens(output("HDMI-A-1", 2 if rotation in (1, 4) else 1, enabled=False), output("DP-1", rotation), output("DP-2", 1))
            self.assertEqual(self.rotate("state").stdout, state + "\n", rotation)

    def test_toggle_to_portrait_rotates_every_enabled_output(self):
        self.screens(output("DP-1", 1), output("HDMI-A-1", 1, enabled=False), output("DP-2", 1), output("eDP-1", 4))
        self.assertEqual(self.rotate("toggle").stdout, "done\n")
        self.assertEqual(self.rotations(), ["output.DP-1.rotation.left", "output.DP-2.rotation.left", "output.eDP-1.rotation.left"])

    def test_toggle_back_to_landscape(self):
        self.screens(output("DP-1", 2))
        self.rotate("toggle")
        self.assertEqual(self.rotations(), ["output.DP-1.rotation.none"])

    def test_toggle_from_upside_down_turns_to_portrait(self):
        self.screens(output("DP-1", 4))
        self.rotate("toggle")
        self.assertEqual(self.rotations(), ["output.DP-1.rotation.left"])

    def test_unknown_argument(self):
        result = self.rotate("flip", code=2)
        self.assertEqual((result.stdout, result.stderr), ("", "Usage: linux-plasma-screen-rotate [state|toggle]\n"))
        self.assertEqual(self.tool_calls(), [])

    def test_unreadable_screens_fail_visibly(self):
        self.box.environment["READ_CODE"] = "1"
        for command in ("state", "toggle"):
            result = self.rotate(command, code=1)
            self.assertEqual((result.stdout, result.stderr),
                             ("", "linux-plasma-screen-rotate: kscreen-doctor could not read the screens\n"))
        self.assertEqual(self.rotations(), [])

    def test_malformed_screen_data_fails_visibly(self):
        self.box.environment["SCREENS"] = "not json"
        for command in ("state", "toggle"):
            result = self.rotate(command, code=1)
            self.assertEqual(result.stdout, "")
            self.assertIn("kscreen-doctor returned unreadable screen data", result.stderr)

    def test_no_enabled_output_fails_visibly(self):
        self.screens(output("DP-1", 1, enabled=False))
        for command in ("state", "toggle"):
            result = self.rotate(command, code=1)
            self.assertEqual((result.stdout, result.stderr), ("", "linux-plasma-screen-rotate: no enabled screen was found\n"))
        self.assertEqual(self.rotations(), [])

    def test_a_failed_rotation_fails_visibly(self):
        self.box.environment["ROTATE_CODE"] = "1"
        self.screens(output("DP-1", 1), output("DP-2", 1))
        result = self.rotate("toggle", code=1)
        self.assertEqual(result.stdout, "")
        self.assertEqual(self.rotations(), ["output.DP-1.rotation.left"])

    def test_missing_jq_fails_visibly(self):
        (self.box.stubs / "jq").unlink()
        self.screens(output("DP-1", 1))
        self.assertEqual(self.rotate("state", code=1).stdout, "")


if __name__ == "__main__":
    unittest.main()
