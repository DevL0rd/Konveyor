#!/usr/bin/env python3
import contextlib
import io
import json
import types
import unittest
from unittest import mock

from collectorharness import WIDGETS, CollectorTest

SCRIPT = WIDGETS / "system-monitor" / "bin" / "sysmon-collect"


class NVMLError(Exception):
    pass


def fake_nvml(fail_init=False):
    def init():
        if fail_init:
            raise NVMLError("NVML Shared Library Not Found")

    def refuse(*_):
        raise NVMLError("Not Supported")

    return types.SimpleNamespace(
        NVMLError=NVMLError, NVML_CLOCK_GRAPHICS=0, NVML_CLOCK_MEM=2, NVML_TEMPERATURE_GPU=0,
        nvmlInit=init, nvmlDeviceGetHandleByIndex=lambda index: "gpu%d" % index,
        nvmlDeviceGetName=lambda handle: b"Test GPU",
        nvmlDeviceGetMemoryInfo=lambda handle: types.SimpleNamespace(used=2 << 30, total=8 << 30),
        nvmlDeviceGetMaxClockInfo=lambda handle, clock: 3000,
        nvmlDeviceGetEnforcedPowerLimit=refuse, nvmlDeviceGetPowerManagementLimit=lambda handle: 175000,
        nvmlDeviceGetPowerManagementDefaultLimit=lambda handle: 150000,
        nvmlDeviceGetUtilizationRates=lambda handle: types.SimpleNamespace(gpu=42),
        nvmlDeviceGetTemperature=lambda handle, sensor: 65, nvmlDeviceGetPowerUsage=lambda handle: 80500,
        nvmlDeviceGetClockInfo=lambda handle, clock: {0: 2100, 2: 10000}[clock], nvmlDeviceGetFanSpeed=refuse)


def stat(cpu, cores):
    lines = ["cpu %d 0 0 %d 0 0 0" % cpu] + ["cpu%d %d 0 0 %d 0 0 0" % (n, busy, idle) for n, (busy, idle) in enumerate(cores)]
    return "\n".join(lines + ["intr 1 2 3", "ctxt 5"]) + "\n"


class TestSysmonCollect(CollectorTest):
    def write_minimal(self):
        self.fake.write("/proc/stat", stat((0, 200), [(0, 100), (0, 100)]))
        self.fake.write("/proc/meminfo", "MemTotal: 8000000 kB\nMemAvailable: 2000000 kB\nSwapTotal: 1000000 kB\nSwapFree: 500000 kB\n")

    def write_hybrid(self):
        self.write_minimal()
        self.fake.write("/proc/stat", stat((0, 400), [(0, 100)] * 4))
        self.fake.write("/proc/cpuinfo", "processor : 0\nmodel name : Test CPU 9000\n")
        self.fake.write("/proc/uptime", "1234.5 99.0\n")
        self.fake.write("/proc/loadavg", "0.5 1.0 1.5 2/300 400\n")
        self.fake.write("/sys/devices/cpu_core/cpus", "0-1\n")
        self.fake.write("/sys/devices/cpu_atom/cpus", "2,3\n")
        for n in range(4):
            base = "/sys/devices/system/cpu/cpu%d/cpufreq/" % n
            self.fake.write(base + "scaling_cur_freq", "4000000\n" if n < 2 else "2000000\n")
            self.fake.write(base + "cpuinfo_max_freq", "5400000\n" if n < 2 else "4000000\n")
        for index, (label, value) in enumerate([("Package id 0", 90000), ("Core 0", 50000), ("Core 8", 70000)], 1):
            self.fake.write("/sys/class/hwmon/hwmon0/temp%d_input" % index, "%d\n" % value)
            self.fake.write("/sys/class/hwmon/hwmon0/temp%d_label" % index, label + "\n")
        self.fake.write("/sys/class/hwmon/hwmon0/name", "coretemp\n")
        self.fake.write("/sys/class/hwmon/hwmon1/name", "asus\n")
        self.fake.write("/sys/class/hwmon/hwmon1/fan1_input", "1500\n")
        self.fake.write("/sys/class/hwmon/hwmon1/fan1_label", "cpu_fan\n")
        self.fake.write("/sys/class/hwmon/hwmon1/fan1_max", "3000\n")
        rapl = "/sys/class/powercap/intel-rapl:0/"
        self.fake.write(rapl + "name", "package-0\n")
        self.fake.write(rapl + "energy_uj", "1000000\n")
        self.fake.write(rapl + "max_energy_range_uj", "100000000\n")
        self.fake.write(rapl + "constraint_0_max_power_uw", "125000000\n")

    def build_twice(self, module, second_stat, energy=None, interval=2.0):
        with mock.patch.object(module.time, "time", return_value=1000.0):
            module.build()
        self.fake.write("/proc/stat", second_stat)
        if energy is not None:
            self.fake.write("/sys/class/powercap/intel-rapl:0/energy_uj", "%d\n" % energy)
        with mock.patch.object(module.time, "time", return_value=1000.0 + interval):
            return module.build()

    def test_splits_a_hybrid_cpu_into_performance_and_efficiency_cores(self):
        self.write_hybrid()
        module = self.load(SCRIPT, "sysmon_collect")
        snapshot = self.build_twice(module, stat((175, 625), [(50, 150), (100, 100), (25, 175), (0, 200)]), energy=11000000)
        cpu = snapshot["cpu"]
        self.assertTrue(cpu["hybrid"])
        self.assertEqual(cpu["p"], [50.0, 100.0])
        self.assertEqual(cpu["e"], [25.0, 0.0])
        self.assertEqual((cpu["p_total"], cpu["e_total"]), (75.0, 12.5))
        self.assertAlmostEqual(cpu["total"], 43.75, delta=0.06)
        self.assertEqual((cpu["p_freq"], cpu["e_freq"], cpu["freq"]), (4.0, 2.0, 3.0))
        self.assertEqual(cpu["temp"], 60)
        self.assertEqual(cpu["fan"], 1500)
        self.assertEqual((cpu["clock_max"], cpu["power_max"], cpu["fan_max"]), (5.4, 125, 3000))
        self.assertEqual(cpu["watts"], 5.0)
        self.assertEqual(snapshot["cpu_model"], "Test CPU 9000")
        self.assertEqual((snapshot["uptime"], snapshot["load"], snapshot["ncpu"]), (1234.5, [0.5, 1.0, 1.5], 4))
        self.assertEqual(snapshot["mem"]["pct"], 75.0)
        self.assertEqual(snapshot["mem"]["swap_pct"], 50.0)
        self.assertIsNone(snapshot["gpu"])

    def test_package_power_survives_the_energy_counter_wrapping(self):
        self.write_hybrid()
        self.fake.write("/sys/class/powercap/intel-rapl:0/energy_uj", "99000000\n")
        module = self.load(SCRIPT, "sysmon_collect")
        snapshot = self.build_twice(module, stat((0, 800), [(0, 200)] * 4), energy=1000000)
        self.assertEqual(snapshot["cpu"]["watts"], 1.0)

    def test_a_cpu_without_per_core_sensors_reports_its_package_temperature(self):
        self.write_hybrid()
        for index in (2, 3):
            (self.home / ("root/sys/class/hwmon/hwmon0/temp%d_input" % index)).unlink()
        module = self.load(SCRIPT, "sysmon_collect")
        self.assertEqual(module.build()["cpu"]["temp"], 90)

    def test_a_machine_without_sensors_or_nvidia_still_reports(self):
        self.write_minimal()
        module = self.load(SCRIPT, "sysmon_collect")
        snapshot = self.build_twice(module, stat((100, 300), [(50, 150), (50, 150)]))
        cpu = snapshot["cpu"]
        self.assertFalse(cpu["hybrid"])
        self.assertEqual(cpu["cores"], [50.0, 50.0])
        self.assertEqual((cpu["temp"], cpu["fan"], cpu["watts"], cpu["freq"]), (0, None, 0, 0.0))
        self.assertEqual((cpu["clock_max"], cpu["power_max"], cpu["fan_max"]), (0.0, 0, 0))
        self.assertEqual((snapshot["cpu_model"], snapshot["uptime"], snapshot["load"]), ("", 0, [0, 0, 0]))
        self.assertIsNone(snapshot["gpu"])

    def test_reads_the_nvidia_gpu_through_nvml(self):
        self.write_minimal()
        module = self.load(SCRIPT, "sysmon_collect", pynvml=fake_nvml())
        gpu = module.build()["gpu"]
        self.assertEqual(gpu, {"name": "Test GPU", "util": 42, "vram_used": 2 << 30, "vram_total": 8 << 30, "vram_pct": 25.0,
                               "temp": 65, "power": 80.5, "clock_gr": 2100, "clock_mem": 10000, "fan": 0,
                               "clock_max": 3000, "power_max": 175})

    def test_nvml_without_an_nvidia_driver_hides_the_gpu(self):
        self.write_minimal()
        module = self.load(SCRIPT, "sysmon_collect", pynvml=fake_nvml(fail_init=True))
        self.assertIsNone(module.build()["gpu"])

    def test_snapshot_before_the_first_write_is_empty(self):
        self.write_minimal()
        module = self.load(SCRIPT, "sysmon_collect")
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            module.cmd_snapshot()
        self.assertEqual(json.loads(output.getvalue()), {"ts": 0, "cpu": {}, "mem": {}, "gpu": None})
        module.write_snapshot({"ts": 5})
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            module.cmd_snapshot()
        self.assertEqual(json.loads(output.getvalue()), {"ts": 5})
        self.assertTrue(str(module.DATA).startswith(str(self.runtime)))

    def test_set_interval_keeps_a_floor_and_rejects_garbage(self):
        self.write_minimal()
        module = self.load(SCRIPT, "sysmon_collect")
        with mock.patch.object(module.sys, "argv", ["sysmon-collect", "--set-interval", "0.1"]):
            self.assertEqual(module.cmd_set_interval(), 0)
        self.assertEqual(json.loads((self.config / "Linux-System-Monitor" / "config.json").read_text()), {"poll_interval": 0.25})
        with mock.patch.object(module.sys, "argv", ["sysmon-collect", "--set-interval", "fast"]):
            self.assertEqual(module.cmd_set_interval(), 1)


if __name__ == "__main__":
    unittest.main()
