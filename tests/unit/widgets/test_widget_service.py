#!/usr/bin/env python3
import contextlib
import io
import shutil
import subprocess
import sys
import unittest
from unittest import mock

from collectorharness import RECORDING_STUB, WIDGETS, CollectorTest, write_stub

SUPERVISOR = WIDGETS / "service" / "konveyor-widgets"
ECORES = WIDGETS / "service" / "konveyor-widgets-ecores"
COLLECTORS = {"system-monitor": "sysmon-collect", "process-monitor": "procmon-collect", "router-monitor": "routermon-collect",
              "system-log": "logmon-collect", "portals": "portal-friends"}
EXITING = RECORDING_STUB + "sys.exit(3)\n"


class Stop(Exception):
    pass


class TestSupervisor(CollectorTest):
    def setUp(self):
        super().setUp()
        self.tree = self.home / "widgets"
        (self.tree / "service").mkdir(parents=True)
        shutil.copy(SUPERVISOR, self.tree / "service" / "konveyor-widgets")

    def collector(self, widget):
        (self.tree / widget / "bin").mkdir(parents=True, exist_ok=True)
        write_stub(self.tree / widget / "bin", COLLECTORS[widget], EXITING)

    def run_supervisor(self, starts):
        module = self.load(self.tree / "service" / "konveyor-widgets", "konveyor_widgets")
        module.RESTART_DELAY = 0.05
        real_start = module.start
        launched = []
        processes = []

        def counted(*arguments):
            if len(launched) == starts:
                raise Stop
            launched.append(arguments[0])
            processes.append(real_start(*arguments))
            return processes[-1]

        errors = io.StringIO()
        with mock.patch.object(module, "start", counted), contextlib.redirect_stderr(errors):
            try:
                code = module.main()
            except Stop:
                code = None
        for process in processes:
            process.wait()
        return code, launched, errors.getvalue()

    def test_exits_with_an_error_when_no_collector_is_installed(self):
        code, launched, errors = self.run_supervisor(starts=5)
        self.assertEqual((code, launched), (1, []))
        self.assertIn("no collectors found", errors)

    def test_starts_every_installed_collector_in_serve_mode(self):
        for widget in COLLECTORS:
            self.collector(widget)
        write_stub(self.tree / "service", "monitor-overlay", RECORDING_STUB)
        self.run_supervisor(starts=5)
        calls = self.calls()
        self.assertEqual(calls[0], ["monitor-overlay", "hide"])
        self.assertEqual(sorted(call[0] for call in calls[1:]), sorted(COLLECTORS.values()))
        self.assertTrue(all(call[1:] == ["--serve"] for call in calls[1:]))

    def test_passes_the_router_collector_its_interpreter_flag(self):
        self.collector("router-monitor")
        module = self.load(self.tree / "service" / "konveyor-widgets", "konveyor_widgets")
        with mock.patch.object(module.subprocess, "Popen") as popen:
            module.start("router-monitor", ["bin/routermon-collect", "--serve"], ["-S"])
        self.assertEqual(popen.call_args.args[0], [sys.executable, "-S", str(self.tree / "router-monitor/bin/routermon-collect"), "--serve"])

    def test_restarts_a_collector_that_exits(self):
        self.collector("system-log")
        code, launched, errors = self.run_supervisor(starts=3)
        self.assertIsNone(code)
        self.assertEqual(launched, ["system-log"] * 3)
        self.assertEqual(errors.count("system-log: collector exited with status 3, restarting"), 3)
        self.assertEqual(len(self.calls()), 3)


class TestEcores(CollectorTest):
    def detect(self):
        module = self.load(ECORES, "konveyor_widgets_ecores")
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            module.main()
        return output.getvalue().strip()

    def clocks(self, *values):
        for cpu, value in enumerate(values):
            self.fake.write("/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq" % cpu, "%d\n" % value)

    def test_intel_hybrid_lists_its_atom_cores(self):
        self.fake.write("/sys/devices/cpu_atom/cpus", "16-31\n")
        self.clocks(5000000, 5000000)
        self.assertEqual(self.detect(), "16-31")

    def test_slower_cores_are_the_efficiency_cores(self):
        self.clocks(5100000, 5100000, 3300000, 3300000, 4800000)
        self.assertEqual(self.detect(), "2,3")

    def test_an_empty_atom_list_falls_through_to_the_clocks(self):
        self.fake.write("/sys/devices/cpu_atom/cpus", "\n")
        self.clocks(5000000, 3000000)
        self.assertEqual(self.detect(), "1")

    def test_prints_nothing_for_uniform_or_unknown_cpus(self):
        self.assertEqual(self.detect(), "")
        self.clocks(4000000, 4000000, 3900000)
        self.assertEqual(self.detect(), "")
        self.fake.write("/sys/devices/system/cpu/cpu1/cpufreq/cpuinfo_max_freq", "garbage\n")
        self.assertEqual(self.detect(), "")

    def test_runs_under_the_installer_interpreter_flags(self):
        result = subprocess.run([sys.executable, "-S", str(ECORES)], capture_output=True, text=True, env=self.environment)
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stderr, "")


if __name__ == "__main__":
    unittest.main()
