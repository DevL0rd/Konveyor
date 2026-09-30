#!/usr/bin/env python3
import contextlib
import io
import json
import os
import subprocess
import types
import unittest
from unittest import mock

from portal_harness import load_script
from router_harness import APP, ROUTER_BIN, RouterTest


SCRIPT = ROUTER_BIN / "routermon-speedtest"
PING = "12 packets transmitted, 12 received, 0% packet loss, time 2203ms\nrtt min/avg/max/mdev = 10.100/12.340/15.000/1.260 ms\n"
LOGGER = """entry = {"tool": os.path.basename(sys.argv[0]), "argv": sys.argv[1:]}
open(os.environ["STUB_LOG"] + ".tools", "a").write(json.dumps(entry) + "\\n")
"""
CURL = LOGGER + """kind = "UP" if "--data-binary" in sys.argv else "DOWN"
sys.stdout.write(os.environ.get("CURL_" + kind, "0"))
sys.exit(int(os.environ.get("CURL_" + kind + "_CODE", "0")))
"""


class Clock:
    def __init__(self, step):
        self.now = -step
        self.step = step

    def __call__(self):
        self.now += self.step
        return self.now


class TestRoutermonSpeedtest(RouterTest):
    stubs = ()

    def setUp(self):
        super().setUp()
        self.script_stub("ping", LOGGER + "sys.stdout.write(os.environ.get('PING_OUTPUT', ''))\n")
        self.script_stub("curl", CURL)
        self.script_stub("head", LOGGER)
        self.box.environment.update(PING_OUTPUT=PING, CURL_DOWN="50000000", CURL_UP="25000000")
        self.module = load_script(SCRIPT, self.box.environment)
        self.live = []
        write_live = self.module.write_live

        def recorded(data):
            write_live(data)
            self.live.append(json.loads((self.runtime / "speedtest_live.json").read_text()))

        self.module.write_live = recorded
        self.module.time = types.SimpleNamespace(monotonic=Clock(2), time=lambda: 1700000000.9, strftime=lambda format: "13:37")

    def speedtest(self):
        output = io.StringIO()
        with mock.patch.dict(os.environ, self.box.environment, clear=True), contextlib.redirect_stdout(output):
            self.module.main()
        return json.loads(output.getvalue())

    def tools(self, tool):
        return [call["argv"] for call in self.tool_calls(tool)]

    def test_measures_ping_download_and_upload(self):
        self.assertEqual(self.speedtest(), {"ok": True, "server": "Cloudflare", "ts": 1700000000, "time": "13:37", "ping_ms": 12.3,
                                            "jitter_ms": 1.3, "loss": 0, "down_mbps": 100.0, "up_mbps": 50.0})
        self.assertEqual(self.tools("ping"), [["-c", "12", "-i", "0.2", "-w", "12", "1.1.1.1"]])
        self.assertEqual(self.tools("curl")[0], ["-s", "-o", "/dev/null", "-w", "%{size_download}", "--max-time", "20",
                                                 "https://speed.cloudflare.com/__down?bytes=50000000"])
        self.assertEqual(self.tools("curl")[2], ["-s", "-o", "/dev/null", "-w", "%{size_upload}", "--max-time", "20",
                                                 "--data-binary", "@-", "https://speed.cloudflare.com/__up"])
        self.assertEqual(self.tools("head"), [["-c", "25000000", "/dev/zero"]] * 2)

    def test_streams_every_phase_to_the_live_file(self):
        result = self.speedtest()
        self.assertEqual(self.live, [
            {"phase": "ping", "mbps": 0.0},
            {"ping_ms": 12.3, "phase": "download", "mbps": 0.0},
            {"ping_ms": 12.3, "phase": "download", "mbps": 100.0},
            {"ping_ms": 12.3, "phase": "download", "mbps": 100.0},
            {"down_mbps": 100.0, "ping_ms": 12.3, "phase": "upload", "mbps": 0.0},
            {"down_mbps": 100.0, "ping_ms": 12.3, "phase": "upload", "mbps": 50.0},
            {"down_mbps": 100.0, "ping_ms": 12.3, "phase": "upload", "mbps": 50.0},
            dict(result, phase="done", mbps=100.0),
        ])
        self.assertEqual(sorted(path.name for path in self.runtime.iterdir()), ["speedtest_live.json"])

    def test_samples_for_the_whole_window(self):
        self.module.time.monotonic = Clock(0.5)
        result = self.speedtest()
        self.assertEqual(len(self.tools("curl")), 16)
        self.assertEqual((result["down_mbps"], result["up_mbps"]), (400.0, 200.0))

    def test_an_unreachable_host_reports_full_loss(self):
        self.box.environment["PING_OUTPUT"] = "12 packets transmitted, 0 received, 100% packet loss, time 11000ms\n"
        result = self.speedtest()
        self.assertEqual((result["ping_ms"], result["jitter_ms"], result["loss"]), (0, 0, 100))

    def test_no_ping_output(self):
        self.box.environment["PING_OUTPUT"] = ""
        result = self.speedtest()
        self.assertEqual((result["ping_ms"], result["jitter_ms"], result["loss"]), (0, 0, 0))

    def test_a_failed_transfer_ends_the_sample(self):
        self.box.environment.update(CURL_DOWN="10000000", CURL_DOWN_CODE="28")
        result = self.speedtest()
        self.assertEqual(result["down_mbps"], 20.0)
        self.assertEqual(len(self.tools("curl")), 3)
        self.assertTrue(result["ok"])

    def test_no_download_is_a_failure(self):
        self.box.environment.update(CURL_DOWN="", CURL_DOWN_CODE="6", CURL_UP="garbage")
        result = self.speedtest()
        self.assertEqual((result["ok"], result["down_mbps"], result["up_mbps"]), (False, 0.0, 0.0))
        self.assertNotIn("error", result)

    def test_an_error_is_reported(self):
        with mock.patch.object(self.module, "sh", side_effect=subprocess.TimeoutExpired("ping", 30)):
            result = self.speedtest()
        self.assertEqual(result, {"ok": False, "server": "Cloudflare", "ts": 1700000000, "time": "13:37",
                                  "error": "Command 'ping' timed out after 30 seconds"})
        self.assertEqual(self.live[-1], dict(result, phase="done", mbps=0))

    def test_live_file_location(self):
        self.assertEqual(self.module.LIVE, str(self.runtime / "speedtest_live.json"))
        self.assertEqual(APP, "Linux-Router-Monitor")


if __name__ == "__main__":
    unittest.main()
