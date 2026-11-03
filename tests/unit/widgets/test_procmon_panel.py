#!/usr/bin/env python3
import contextlib
import io
import json
import os
import runpy
import unittest
from unittest import mock

from collectorharness import WIDGETS, CollectorTest, load_script

SCRIPT = WIDGETS / "process-monitor" / "bin" / "procmon-collect"
APP = "Linux-Process-Mon"


class TestProcmonPanel(CollectorTest):
    def setUp(self):
        super().setUp()
        self.module = self.load(SCRIPT, "procmon_collect_panel")
        self.rundir = self.runtime / APP
        self.panel = self.rundir / "panel" / "panel.json"

    def snapshot(self, *rows):
        return {"procs": list(rows), "mem_total": 100, "vram_total": 50, "ncpu": 2, "plasma_pid": 0}

    def test_the_busiest_graphics_process_is_the_summary_leader(self):
        rows = [{"pid": 1, "name": "a", "gpu": 10, "cpu": 1.5, "vram": 5}, {"pid": 2, "name": "b", "gpu": 30, "vram": 7},
                {"pid": 3, "name": "c", "gpu": 20}, {"pid": 4, "name": "d"}]
        self.module.panel_snapshot(self.snapshot(*rows))
        panel = json.loads(self.panel.read_text())
        self.assertEqual(panel["focus"], None)
        self.assertEqual(panel["summary"], {"count": 4, "cpu": 1.5, "gpu": 60, "vram": 12, "memTotal": 100, "vramTotal": 50, "ncpu": 2,
                                            "gpuTop": {"pid": 2, "name": "b", "gpu": 30}})

    def test_an_unchanged_summary_is_not_written_again(self):
        snapshot = self.snapshot({"pid": 1, "name": "a", "gpu": 10})
        with mock.patch.object(self.module.os, "replace", wraps=os.replace) as replace:
            self.module.panel_snapshot(snapshot)
            self.module.panel_snapshot(snapshot)
            self.module.panel_snapshot(self.snapshot({"pid": 1, "name": "a", "gpu": 11}))
        self.assertEqual(replace.call_count, 2)

    def test_a_focus_file_that_is_not_a_process_id_means_no_focus(self):
        self.rundir.mkdir()
        (self.rundir / "focus").write_text("not a number\nname\n")
        self.assertEqual(self.module._read_focus(), 0)
        self.assertEqual(self.module._panel["focus_name"], "")

    def test_the_snapshot_is_written_compactly(self):
        self.module.write_snapshot({"ts": 1, "procs": []})
        self.assertEqual((self.rundir / "data.json").read_text(), '{"ts":1,"procs":[]}')

    def test_an_unwritten_snapshot_reads_as_empty(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            self.module.cmd_snapshot()
        self.assertEqual(json.loads(output.getvalue()), {"ts": 0, "ncpu": 1, "procs": []})

    def test_the_poll_interval_is_saved_and_garbage_refused(self):
        for argv, code in ((["procmon-collect", "--set-interval", "1.5"], 0), (["procmon-collect", "--set-interval"], 1)):
            with mock.patch.object(self.module.sys, "argv", argv):
                self.assertEqual(self.module.cmd_set_interval(), code)
        self.assertEqual(json.loads((self.config / APP / "config.json").read_text()), {"poll_interval": 1.5})

    def test_the_script_exits_with_the_status_of_its_command(self):
        output = io.StringIO()
        with mock.patch("sys.argv", ["procmon-collect", "--snapshot"]), contextlib.redirect_stdout(output), self.assertRaises(SystemExit) as raised:
            runpy.run_path(str(SCRIPT), run_name="__main__")
        self.assertEqual(raised.exception.code, 0)
        self.assertEqual(json.loads(output.getvalue())["procs"], [])

    def test_the_file_limit_is_raised_to_the_hard_limit(self):
        with mock.patch("resource.getrlimit", return_value=(1024, 524288)), mock.patch("resource.setrlimit") as setrlimit:
            load_script(SCRIPT, "procmon_collect_limit")
        setrlimit.assert_called_once_with(self.module.resource.RLIMIT_NOFILE, (524288, 524288))

    def test_a_refused_file_limit_is_ignored(self):
        with mock.patch("resource.getrlimit", return_value=(1024, 524288)), mock.patch("resource.setrlimit", side_effect=ValueError):
            self.assertTrue(load_script(SCRIPT, "procmon_collect_refused").MEM_TOTAL >= 0)


if __name__ == "__main__":
    unittest.main()
