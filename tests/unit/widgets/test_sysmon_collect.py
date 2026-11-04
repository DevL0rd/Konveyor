#!/usr/bin/env python3
import contextlib
import io
import json
import types
import unittest
from unittest import mock

from collectorharness import WIDGETS, CollectorTest, ServeLoopTests, run_main

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


class TestSysmonCollect(ServeLoopTests, CollectorTest):
    APP = "Linux-System-Monitor"
    DEFAULT_INTERVAL = 2.0

    def serving_module(self):
        self.write_minimal()
        return self.load(SCRIPT, "sysmon_collect")

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
        with mock.patch.object(module.time, "time", return_value=1000.0), mock.patch.object(module.time, "monotonic", return_value=50.0):
            module.build()
        self.fake.write("/proc/stat", second_stat)
        if energy is not None:
            self.fake.write("/sys/class/powercap/intel-rapl:0/energy_uj", "%d\n" % energy)
        with mock.patch.object(module.time, "time", return_value=1000.0 + interval), \
                mock.patch.object(module.time, "monotonic", return_value=50.0 + interval):
            return module.build()

    def test_cpu_lines_that_are_not_a_core_number_are_skipped(self):
        module = self.serving_module()
        self.fake.write("/proc/stat", stat((0, 200), [(0, 100)]).replace("intr", "cpufreq 9 9 9 9 9 9 9\nintr"))
        self.assertEqual(sorted(module.read_stat()), ["cpu", "cpu0"])

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

    def test_offline_cpus_leave_the_online_cores_readings_in_place(self):
        self.write_hybrid()
        def sparse(busy, rounds):
            lines = ["cpu 0 0 0 %d 0 0 0\n" % (300 * rounds - sum(busy.values()))]
            return "".join(lines + ["cpu%d %d 0 0 %d 0 0 0\n" % (n, u, 100 * rounds - u) for n, u in busy.items()])
        idle, loaded = sparse({0: 0, 2: 0, 3: 0}, 1), sparse({0: 50, 2: 80, 3: 20}, 2)
        self.fake.write("/proc/stat", idle)
        cpu = self.build_twice(self.load(SCRIPT, "sysmon_collect"), loaded)["cpu"]
        self.assertEqual((cpu["p"], cpu["e"]), ([50.0], [80.0, 20.0]))
        self.fake.write("/sys/devices/cpu_core/cpus", "")
        self.fake.write("/proc/stat", idle)
        snapshot = self.build_twice(self.load(SCRIPT, "sysmon_collect_flat"), loaded)
        self.assertEqual((snapshot["cpu"]["cores"], snapshot["ncpu"]), ([50.0, 80.0, 20.0], 3))

    def test_an_unreadable_frequency_sensor_does_not_stop_the_snapshots(self):
        self.write_hybrid()
        self.fake.write("/sys/devices/system/cpu/cpu3/cpufreq/scaling_cur_freq", "<unknown>\n")
        module = self.load(SCRIPT, "sysmon_collect")
        cpu = module.build()["cpu"]
        self.assertEqual((cpu["p_freq"], cpu["freq"]), (4.0, 3.33))

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

    def write_amdgpu(self, card, revision, vram, gtt):
        dev = "/sys/class/drm/%s/device/" % card
        files = {"gpu_busy_percent": "37", "mem_info_vram_used": vram[0], "mem_info_vram_total": vram[1],
                 "mem_info_gtt_used": gtt[0], "mem_info_gtt_total": gtt[1], "power/runtime_status": "active",
                 "pp_dpm_sclk": "0: 600Mhz \n1: 806Mhz *\n2: 2900Mhz ", "hwmon/hwmon5/temp1_input": "55000",
                 "hwmon/hwmon5/power1_average": "12345678", "hwmon/hwmon5/freq1_input": "1800000000"}
        for name, text in files.items():
            self.fake.write(dev + name, "%s\n" % text)
        (self.home / ("root" + dev + "gpu_metrics")).write_bytes(bytes([0x08, 0x01, revision, 0]))
        return dev

    def test_an_amd_apu_reports_vram_and_gtt_as_gpu_memory(self):
        self.write_minimal()
        self.fake.write("/proc/cpuinfo", "model name : AMD Ryzen AI 7 350 w/ Radeon 860M\n")
        self.write_amdgpu("card1", 3, (1 << 29, 1 << 29), (1 << 30, 3 << 30))
        gpu = self.load(SCRIPT, "sysmon_collect").build()["gpu"]
        self.assertEqual(gpu, {"name": "AMD Radeon 860M", "util": 37, "vram_used": 3 << 29, "vram_total": 7 << 29,
                               "vram_pct": 3 / 7 * 100, "temp": 55.0, "power": 12.3, "clock_gr": 1800, "clock_mem": 0,
                               "fan": 0, "clock_max": 2900, "power_max": 0, "shared": True})

    def test_a_discrete_amd_gpu_wins_over_the_apu_and_counts_only_vram(self):
        self.write_minimal()
        self.fake.write("/proc/cpuinfo", "model name : AMD Ryzen AI 7 350 w/ Radeon 860M\n")
        self.write_amdgpu("card1", 3, (1 << 29, 1 << 29), (0, 1 << 30))
        dev = self.write_amdgpu("card0", 1, (2 << 30, 16 << 30), (1 << 30, 32 << 30))
        self.fake.write(dev + "uevent", "DRIVER=amdgpu\nPCI_SLOT_NAME=0000:03:00.0\n")
        self.fake.write("/run/udev/data/+pci:0000:03:00.0", "E:ID_MODEL_FROM_DATABASE=Navi 32 [Radeon RX 7700 XT / 7800 XT]\n")
        self.fake.write(dev + "hwmon/hwmon5/pwm1", "64\n")
        self.fake.write(dev + "hwmon/hwmon5/pwm1_max", "255\n")
        self.fake.write(dev + "hwmon/hwmon5/power1_cap", "263000000\n")
        module = self.load(SCRIPT, "sysmon_collect")
        gpu = module.build()["gpu"]
        self.assertEqual((gpu["name"], gpu["vram_used"], gpu["vram_total"], gpu["shared"]),
                         ("AMD Radeon RX 7700 XT / 7800 XT", 2 << 30, 16 << 30, False))
        self.assertEqual((gpu["fan"], gpu["power_max"]), (25, 263))
        self.fake.write(dev + "power/runtime_status", "suspended\n")
        gpu = module.build()["gpu"]
        self.assertEqual((gpu["util"], gpu["temp"], gpu["power"], gpu["vram_used"], gpu["vram_total"]), (0, 0, 0, 0, 16 << 30))

    def test_nvml_takes_precedence_over_amdgpu(self):
        self.write_minimal()
        self.write_amdgpu("card1", 3, (1 << 29, 1 << 29), (0, 1 << 30))
        self.assertEqual(self.load(SCRIPT, "sysmon_collect", pynvml=fake_nvml()).build()["gpu"]["name"], "Test GPU")

    def write_net(self, counters):
        lines = ["Inter-|   Receive |  Transmit", " face |bytes packets|bytes packets"]
        for name, (rx, tx) in counters.items():
            lines.append("%6s: %d 0 0 0 0 0 0 0 %d 0 0 0 0 0 0 0" % (name, rx, tx))
        self.fake.write("/proc/net/dev", "\n".join(lines) + "\n")

    def test_network_throughput_sums_the_physical_interfaces(self):
        self.write_minimal()
        for nic in ("enp5s0", "wlan0"):
            self.fake.write("/sys/class/net/%s/device/uevent" % nic, "")
        self.write_net({"lo": (0, 0), "enp5s0": (1000, 500), "wlan0": (0, 0), "tailscale0": (0, 0)})
        module = self.load(SCRIPT, "sysmon_collect")
        with mock.patch.object(module.time, "monotonic", return_value=50.0):
            module.build()
        self.write_net({"lo": (9 << 20, 9 << 20), "enp5s0": (5000, 1500), "wlan0": (2000, 0), "tailscale0": (4000, 4000)})
        with mock.patch.object(module.time, "monotonic", return_value=52.0):
            net = module.build()["net"]
        self.assertEqual(net, {"down": 3000, "up": 500, "ifaces": ["enp5s0", "wlan0"]})
        self.write_net({"lo": (0, 0), "enp5s0": (100, 100), "wlan0": (4000, 200), "tailscale0": (0, 0)})
        with mock.patch.object(module.time, "monotonic", return_value=54.0):
            self.assertEqual(module.build()["net"]["down"], 1000)

    def test_network_throughput_averages_over_two_seconds_at_any_poll_interval(self):
        self.write_minimal()
        self.fake.write("/sys/class/net/enp5s0/device/uevent", "")
        module = self.load(SCRIPT, "sysmon_collect")
        downs = []
        for step, received in enumerate([0, 0, 4000, 4000, 4000, 4000, 4000]):
            self.write_net({"enp5s0": (received, 0)})
            with mock.patch.object(module.time, "monotonic", return_value=50.0 + step * 0.5):
                downs.append(module.build()["net"]["down"])
        # A 4000-byte burst in one half-second poll reads as 2000 B/s over the
        # two-second window until the window has moved past it.
        self.assertEqual(downs, [0, 0, 4000, 2667, 2000, 2000, 0])

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

    def test_once_prints_a_fresh_reading(self):
        self.write_minimal()
        code, output, _ = run_main(self.load(SCRIPT, "sysmon_collect"), "sysmon-collect", "--once")
        self.assertIn(code, (None, 0))
        self.assertIn("cpu", json.loads(output))
        self.assertIn("mem", json.loads(output))

    def test_snapshot_mode_prints_the_last_snapshot(self):
        self.write_minimal()
        module = self.load(SCRIPT, "sysmon_collect")
        module.write_snapshot({"ts": 7})
        self.assertEqual(json.loads(run_main(module, "sysmon-collect", "--snapshot")[1]), {"ts": 7})

    def test_an_unknown_mode_prints_usage(self):
        self.write_minimal()
        code, output, errors = run_main(self.load(SCRIPT, "sysmon_collect"), "sysmon-collect", "--twice")
        self.assertEqual((code, output), (2, ""))
        self.assertIn("usage: sysmon-collect", errors)


if __name__ == "__main__":
    unittest.main()
