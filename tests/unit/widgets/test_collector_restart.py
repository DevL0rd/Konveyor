#!/usr/bin/env python3
import os
import signal
import subprocess
import sys
import unittest
from unittest import mock

from collectorharness import RECORDING_STUB, WIDGETS, CollectorTest, load_script, run_main, write_stub

COLLECTORS = {"procmon-collect": (WIDGETS / "process-monitor" / "bin" / "procmon-collect", "Linux-Process-Mon"),
              "routermon-collect": (WIDGETS / "router-monitor" / "bin" / "routermon-collect", "Linux-Router-Monitor")}
SLEEPER = "#!/usr/bin/env python3\nimport time\ntime.sleep(120)\n"


class Stop(Exception):
    pass


class TestCollectorRestart(CollectorTest):
    def setUp(self):
        super().setUp()
        write_stub(self.stubs, "systemctl", RECORDING_STUB)

    def collectors(self):
        for name, (script, app) in COLLECTORS.items():
            with self.subTest(collector=name):
                self.log.unlink(missing_ok=True)
                module = self.load(script, name.replace("-", "_"))
                yield name, module, self.runtime / app

    def serving(self, rundir, pid, *arguments):
        rundir.mkdir(exist_ok=True)
        (rundir / "serve.pid").write_text(str(pid))
        if arguments:
            (self.home / "root" / "proc" / str(pid)).mkdir(parents=True, exist_ok=True)
            (self.home / "root" / "proc" / str(pid) / "cmdline").write_bytes(b"\0".join(argument.encode() for argument in arguments) + b"\0")

    def restart(self, module, name):
        with mock.patch.object(module.os, "kill") as kill:
            code = run_main(module, name, "--restart")[0]
        return code, [call.args for call in kill.call_args_list]

    def test_serving_records_the_collector_pid(self):
        for name, module, rundir in self.collectors():
            first = "build" if name == "procmon-collect" else "load_config"
            with mock.patch.object(module, first, side_effect=Stop), self.assertRaises(Stop):
                run_main(module, name, "--serve")
            self.assertEqual((rundir / "serve.pid").read_text(), str(os.getpid()))

    def test_a_running_collector_is_stopped_for_the_service_to_restart(self):
        for name, module, rundir in self.collectors():
            self.serving(rundir, 4242, "/usr/bin/python3", "-S", "/opt/widgets/bin/" + name, "--serve")
            self.assertEqual(self.restart(module, name), (0, [(4242, signal.SIGTERM)]))
            self.assertEqual(self.calls(), [])

    def test_without_a_running_collector_the_service_is_started(self):
        cases = {"no pid file": None, "a stale pid": (), "another program": ("sleep", "60"),
                 "the collector in another mode": ("python3", "/opt/widgets/bin/procmon-collect", "--once"),
                 "another collector": ("python3", "/opt/widgets/bin/sysmon-collect", "--serve")}
        for name, module, rundir in self.collectors():
            for case, arguments in cases.items():
                with self.subTest(case=case):
                    self.log.unlink(missing_ok=True)
                    (rundir / "serve.pid").unlink(missing_ok=True)
                    if arguments is not None:
                        self.serving(rundir, 5151, *arguments)
                    self.assertEqual(self.restart(module, name), (0, []))
                    self.assertEqual(self.calls(), [["systemctl", "--user", "start", "konveyor-widgets.service"]])

    def test_a_failing_service_start_is_reported(self):
        write_stub(self.stubs, "systemctl", RECORDING_STUB + "sys.exit(5)\n")
        for name, module, _ in self.collectors():
            self.assertEqual(self.restart(module, name), (5, []))

    def test_restart_stops_a_real_hung_collector(self):
        self.start(mock.patch.dict(os.environ, self.environment, clear=True))
        (self.home / "hung").mkdir()
        for name, (script, app) in COLLECTORS.items():
            with self.subTest(collector=name):
                hung = write_stub(self.home / "hung", name, SLEEPER)
                process = subprocess.Popen([sys.executable, str(hung), "--serve"])
                self.addCleanup(process.kill)
                rundir = self.runtime / app
                rundir.mkdir(exist_ok=True)
                (rundir / "serve.pid").write_text(str(process.pid))
                module = load_script(script, name.replace("-", "_") + "_real")
                self.assertEqual(run_main(module, name, "--restart")[0], 0)
                self.assertEqual(process.wait(timeout=10), -signal.SIGTERM)


if __name__ == "__main__":
    unittest.main()
