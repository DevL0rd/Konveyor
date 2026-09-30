#!/usr/bin/env python3
import ctypes
import json
import os
import types
import unittest
from unittest import mock

from collectorharness import WIDGETS, CollectorTest, ServeLoopTests, run_main

SCRIPT = WIDGETS / "process-monitor" / "bin" / "procmon-collect"


def stat_line(pid, comm, ppid, ticks, threads, rss_pages, start=100):
    fields = ["S", ppid, 0, 0, 0, 0, 0, 0, 0, 0, 0, ticks, 0, 0, 0, 20, 0, threads, 0, start, 0, rss_pages, 0, 0]
    return "%d (%s) %s\n" % (pid, comm, " ".join(str(field) for field in fields))


class Sample(ctypes.Structure):
    _fields_ = [("pid", ctypes.c_uint), ("timeStamp", ctypes.c_ulonglong), ("smUtil", ctypes.c_uint), ("memUtil", ctypes.c_uint),
                ("encUtil", ctypes.c_uint), ("decUtil", ctypes.c_uint)]


class ProcessInfo(ctypes.Structure):
    _fields_ = [("pid", ctypes.c_uint), ("usedGpuMemory", ctypes.c_ulonglong), ("gpuInstanceId", ctypes.c_uint),
                ("computeInstanceId", ctypes.c_uint)]


def fake_process_nvml(samples, compute, graphics):
    missing = ctypes.c_ulonglong(2 ** 64 - 1)

    def fill(buffer, count, rows):
        count._obj.value, needed = len(rows), len(rows)
        if needed > len(buffer):
            return 7
        for index, row in enumerate(rows):
            buffer[index] = row
        return 0

    functions = {
        "nvmlDeviceGetProcessUtilization": lambda handle, buffer, count, since: fill(buffer, count, [Sample(*row) for row in samples]),
        "nvmlDeviceGetComputeRunningProcesses_v3": lambda handle, count, buffer: fill(
            buffer, count, [ProcessInfo(pid, missing.value if used is None else used) for pid, used in compute]),
        "nvmlDeviceGetGraphicsRunningProcesses_v3": lambda handle, count, buffer: fill(
            buffer, count, [ProcessInfo(pid, used) for pid, used in graphics]),
    }
    return types.SimpleNamespace(
        NVML_SUCCESS=0, NVML_ERROR_INSUFFICIENT_SIZE=7, NVML_VALUE_NOT_AVAILABLE_ulonglong=missing,
        c_nvmlProcessUtilizationSample_t=Sample, c_nvmlProcessInfo_v3_t=ProcessInfo,
        nvmlInit=lambda: None, nvmlDeviceGetHandleByIndex=lambda index: "gpu0",
        nvmlDeviceGetMemoryInfo=lambda handle: types.SimpleNamespace(total=8 << 30),
        _nvmlGetFunctionPointer=functions.__getitem__, _nvmlCheckReturn=lambda code: None)


class TestProcmonCollect(ServeLoopTests, CollectorTest):
    APP = "Linux-Process-Mon"
    DEFAULT_INTERVAL = 0.5

    def serving_module(self):
        self.process(1, "init", 0)
        return self.module()

    def setUp(self):
        super().setUp()
        self.fake.write("/proc/meminfo", "MemTotal: 16000000 kB\nMemFree: 1 kB\n")
        self.applications = self.home / "applications"
        self.applications.mkdir()

    def process(self, pid, comm, ppid, ticks=0, threads=1, rss_pages=1, io=None, environ=b""):
        self.fake.write("/proc/%d/stat" % pid, stat_line(pid, comm, ppid, ticks, threads, rss_pages))
        if io is not None:
            self.fake.write("/proc/%d/io" % pid, "rchar: 1\nread_bytes: %d\nwrite_bytes: %d\ncancelled_write_bytes: 0\n" % io)
        (self.home / "root" / "proc" / str(pid) / "environ").write_bytes(environ)

    def module(self, pynvml=None):
        module = self.load(SCRIPT, "procmon_collect", pynvml=pynvml)
        module.icons.DESKTOP_DIRS = [str(self.applications)]
        module.CLK, module.proc.PAGE, module.NCPU = 100, 4096, 2
        module.graphics._frame_telemetry["retry"] = float("inf")
        return module

    def build_at(self, module, when):
        with mock.patch.object(module.time, "time", return_value=when), mock.patch.object(module.time, "monotonic", return_value=when):
            return module.build()

    def rows(self, snapshot):
        return {row["pid"]: row for row in snapshot["procs"]}

    def test_reports_cpu_memory_disk_and_threads_per_process(self):
        self.process(1, "systemd", 0, ticks=0, io=(0, 0))
        self.process(50, "game", 1, ticks=0, threads=4, rss_pages=256, io=(1000, 0))
        module = self.module()
        self.build_at(module, 100.0)
        self.process(50, "game", 1, ticks=100, threads=4, rss_pages=256, io=(3000, 1000))
        rows = self.rows(self.build_at(module, 102.0))
        self.assertEqual(rows[50]["cpu"], 25.0)
        self.assertEqual(rows[50]["ram"], 256 * 4096)
        self.assertEqual(rows[50]["threads"], 4)
        self.assertEqual(rows[50]["disk"], 1500)
        self.assertNotIn("cpu", rows[1])
        self.assertEqual(json.loads(json.dumps(rows[1]))["name"], "systemd")

    def test_parents_carry_the_totals_of_their_children(self):
        self.process(1, "init", 0, threads=1, rss_pages=1, io=(0, 0))
        self.process(10, "steam", 1, threads=2, rss_pages=10, io=(0, 0))
        self.process(11, "game", 10, threads=3, rss_pages=20, io=(0, 0))
        rows = self.rows(self.build_at(self.module(), 100.0))
        self.assertEqual(rows[10]["athreads"], 5)
        self.assertEqual(rows[10]["aram"], 30 * 4096)
        self.assertEqual(rows[1]["athreads"], 6)
        self.assertEqual(rows[11]["athreads"], 3)

    def test_kernel_threads_are_marked(self):
        self.process(2, "kthreadd", 0)
        self.process(3, "kworker/0:1", 2)
        self.process(4, "bash", 1)
        rows = self.rows(self.build_at(self.module(), 100.0))
        self.assertTrue(rows[2]["kernel"] and rows[3]["kernel"])
        self.assertNotIn("kernel", rows[4])

    def test_names_with_spaces_and_parentheses_are_kept_whole(self):
        self.process(7, "Web Content (x)", 1)
        self.assertEqual(self.rows(self.build_at(self.module(), 100.0))[7]["name"], "Web Content (x)")

    def test_icons_come_from_desktop_files_and_aliases(self):
        (self.applications / "org.kde.konsole.desktop").write_text(
            "[Desktop Entry]\nName=Konsole\nExec=konsole %u\nIcon=utilities-terminal\n[Desktop Action new]\nIcon=other\n")
        (self.applications / "hidden.desktop").write_text("[Desktop Entry]\nExec=env FOO=1 python3 tool.py\nIcon=\n")
        self.process(20, "konsole", 1)
        self.process(21, "chrome", 1)
        self.process(22, "python3", 1)
        rows = self.rows(self.build_at(self.module(), 100.0))
        self.assertEqual(rows[20]["icon"], "utilities-terminal")
        self.assertEqual(rows[21]["icon"], "google-chrome")
        self.assertNotIn("icon", rows[22])

    def test_unreadable_io_counts_as_no_disk_activity(self):
        self.process(30, "secret", 1, io=(5, 5))
        io = self.home / "root" / "proc" / "30" / "io"
        module = self.module()
        self.build_at(module, 100.0)
        io.chmod(0)
        self.addCleanup(io.chmod, 0o644)
        if os.access(io, os.R_OK):
            self.skipTest("running as root, so the file stays readable")
        self.process(30, "secret", 1, ticks=10)
        module.proc._io_fds.clear()
        module.proc._io_cache.clear()
        row = self.rows(self.build_at(module, 101.0))[30]
        self.assertNotIn("disk", row)
        self.assertEqual(row["cpu"], 5.0)

    def test_a_vanished_process_drops_out(self):
        self.process(40, "short", 1, io=(0, 0))
        module = self.module()
        self.assertIn(40, self.rows(self.build_at(module, 100.0)))
        for path in (self.home / "root" / "proc" / "40").iterdir():
            path.unlink()
        (self.home / "root" / "proc" / "40").rmdir()
        self.assertNotIn(40, self.rows(self.build_at(module, 101.0)))
        self.assertEqual(module.proc._stat_fds, {})

    def test_the_focused_steam_game_takes_its_library_name_and_icon(self):
        (self.applications / "Portal 2.desktop").write_text("[Desktop Entry]\nName=Portal 2\nExec=steam steam://rungameid/620\nIcon=steam_icon_620\n")
        self.process(60, "portal2_linux", 1, environ=b"HOME=/x\0SteamAppId=620\0")
        self.process(61, "wine64", 1, environ=b"STEAM_COMPAT_APP_ID=999\0")
        module = self.module()
        (self.runtime / "Linux-Process-Mon").mkdir()
        focus = self.runtime / "Linux-Process-Mon" / "focus"
        focus.write_text("60\nPortal 2 window\n")
        snapshot = self.build_at(module, 100.0)
        self.assertEqual((self.rows(snapshot)[60]["name"], self.rows(snapshot)[60]["icon"]), ("Portal 2", "steam_icon_620"))
        focus.write_text("61\n")
        os.utime(focus, ns=(1, 1))
        self.assertEqual(self.rows(self.build_at(module, 101.0))[61]["icon"], "steam_icon_999")

    def test_the_panel_snapshot_summarises_the_focused_process(self):
        self.process(1, "init", 0, io=(0, 0))
        self.process(70, "app", 1, threads=2, rss_pages=100, io=(0, 0))
        self.process(71, "helper", 70, threads=1, rss_pages=50, io=(0, 0))
        module = self.module()
        (self.runtime / "Linux-Process-Mon").mkdir()
        (self.runtime / "Linux-Process-Mon" / "focus").write_text("70\n")
        module.panel_snapshot(self.build_at(module, 100.0))
        panel = json.loads((self.runtime / "Linux-Process-Mon" / "panel" / "panel.json").read_text())
        self.assertEqual(panel["summary"]["count"], 3)
        self.assertEqual(panel["summary"]["memTotal"], 16000000 * 1024)
        self.assertEqual(panel["focus"]["pid"], 70)
        self.assertEqual(panel["focus"]["parentName"], "init")
        self.assertEqual(panel["focus"]["ram"], 150 * 4096)
        self.assertEqual(panel["focus"]["threads"], 3)
        self.assertEqual(panel["focus"]["fps"], -1)

    def test_frame_rates_from_a_child_show_on_the_focused_parent(self):
        self.process(1, "init", 0)
        self.process(80, "steam", 1)
        self.process(81, "game", 80)
        module = self.module()
        frames = {81: {"fps": 144, "frametime": 6.94, "fps_low": 120}}
        with mock.patch.object(module.graphics, "frame_rates", return_value=frames):
            snapshot = self.build_at(module, 100.0)
        (self.runtime / "Linux-Process-Mon").mkdir()
        (self.runtime / "Linux-Process-Mon" / "focus").write_text("80\n")
        module.panel_snapshot(snapshot)
        focus = json.loads((self.runtime / "Linux-Process-Mon" / "panel" / "panel.json").read_text())["focus"]
        self.assertEqual((focus["fps"], focus["fpsLow"], focus["framePid"]), (144, 120, 81))

    def test_frame_telemetry_without_a_bus_reports_nothing_and_backs_off(self):
        module = self.module()
        module.graphics._frame_telemetry["retry"] = 0.0
        self.assertEqual(module.graphics.frame_rates(), {})
        self.assertGreater(module.graphics._frame_telemetry["retry"], 0.0)

    def test_gpu_columns_show_each_processes_share_even_with_more_processes_than_the_buffers_hold(self):
        self.process(1, "init", 0)
        self.process(90, "game", 1)
        self.process(91, "encoder", 1)
        self.process(92, "cuda", 1)
        samples = [(90, 5, 30, 0, 0, 0), (90, 9, 60, 0, 0, 0), (91, 9, 5, 0, 40, 20)] + [(1000 + n, 1, 1, 0, 0, 0) for n in range(80)]
        nvml = fake_process_nvml(samples, compute=[(92, None)], graphics=[(90, 2 << 30)] + [(2000 + n, 1) for n in range(70)])
        rows = self.rows(self.build_at(self.module(pynvml=nvml), 100.0))
        self.assertEqual((rows[90]["gpu"], rows[90]["vram"]), (60, 2 << 30))
        self.assertEqual((rows[91]["gpu"], rows[91]["enc"], rows[91]["dec"]), (5, 40, 20))
        self.assertNotIn("vram", rows[92])
        self.assertEqual(rows[1]["agpu"], 65)
        self.assertEqual(self.build_at(self.module(pynvml=nvml), 100.0)["vram_total"], 8 << 30)

    def test_no_nvidia_reports_no_gpu_columns(self):
        self.process(90, "app", 1)
        snapshot = self.build_at(self.module(), 100.0)
        self.assertEqual(snapshot["vram_total"], 0)
        self.assertNotIn("gpu", self.rows(snapshot)[90])

    def test_bytes_text(self):
        module = self.module()
        self.assertEqual([module._bytes_text(value) for value in (5, 2048, 3 << 20, 3 << 30, 2 << 40)],
                         ["5B", "2K", "3M", "3.0G", "2.0T"])

    def test_once_prints_a_fresh_reading(self):
        self.process(1, "init", 0, ticks=5)
        code, output, _ = run_main(self.module(), "procmon-collect", "--once")
        self.assertIn(code, (None, 0))
        self.assertIn(1, self.rows(json.loads(output)))

    def test_snapshot_mode_prints_the_last_snapshot(self):
        module = self.module()
        os.makedirs(module.RUNDIR, exist_ok=True)
        with open(module.DATA, "w") as handle:
            json.dump({"ts": 9, "procs": []}, handle)
        self.assertEqual(json.loads(run_main(module, "procmon-collect", "--snapshot")[1]), {"ts": 9, "procs": []})

    def test_an_unknown_mode_prints_usage(self):
        code, output, errors = run_main(self.module(), "procmon-collect", "--twice")
        self.assertEqual((code, output), (2, ""))
        self.assertIn("usage: procmon-collect", errors)


if __name__ == "__main__":
    unittest.main()
