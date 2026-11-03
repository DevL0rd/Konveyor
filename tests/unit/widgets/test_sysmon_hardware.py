#!/usr/bin/env python3
import contextlib
import io
import runpy
import unittest
from unittest import mock

from collectorharness import WIDGETS, CollectorTest
from test_sysmon_collect import stat

SCRIPT = WIDGETS / "system-monitor" / "bin" / "sysmon-collect"
RAPL = "/sys/class/powercap/intel-rapl:%d/"
HWMON = "/sys/class/hwmon/hwmon%d/"


class TestSysmonHardware(CollectorTest):
    def setUp(self):
        super().setUp()
        self.fake.write("/proc/stat", stat((0, 200), [(0, 100), (0, 100)]))
        self.fake.write("/proc/meminfo", "MemTotal: 8000000 kB\nMemAvailable: 2000000 kB\nSwapTotal: 0 kB\nSwapFree: 0 kB\n")

    def module(self):
        return self.load(SCRIPT, "sysmon_collect_hardware")

    def rapl(self, index, name, **files):
        if name is not None:
            self.fake.write(RAPL % index + "name", name + "\n")
        for file, value in files.items():
            self.fake.write(RAPL % index + file, "%s\n" % value)

    def build_at(self, module, when):
        with mock.patch.object(module.time, "time", return_value=when), mock.patch.object(module.time, "monotonic", return_value=when):
            return module.build()

    def test_the_clock_ceiling_ignores_unreadable_cpus(self):
        self.fake.write("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "5400000\n")
        self.fake.write("/sys/devices/system/cpu/cpu1/cpufreq/cpuinfo_max_freq", "<unknown>\n")
        self.assertEqual(self.module().CPU_CLOCK_MAX, 5.4)

    def test_the_power_ceiling_comes_from_the_package_domain_only(self):
        self.rapl(0, "dram", constraint_0_max_power_uw=999000000)
        self.assertEqual(self.module().CPU_POWER_MAX, 0)
        self.rapl(1, "package-0", constraint_0_max_power_uw="garbage", constraint_1_max_power_uw=125000000)
        self.assertEqual(self.module().CPU_POWER_MAX, 125)

    def test_a_domain_without_a_name_gives_no_power_ceiling_and_no_energy_counter(self):
        self.rapl(0, None, constraint_0_max_power_uw=888000000, energy_uj=1)
        module = self.module()
        self.assertEqual((module.CPU_POWER_MAX, module.RAPL_ENERGY, module.RAPL_MAX), (0, None, None))

    def test_a_package_domain_without_readable_limits_has_no_power_ceiling(self):
        self.rapl(0, "package-0", constraint_0_max_power_uw="garbage")
        self.assertEqual(self.module().CPU_POWER_MAX, 0)

    def test_the_energy_counter_is_the_package_domains(self):
        self.rapl(1, "package-0", energy_uj=7, max_energy_range_uj=100)
        module = self.module()
        self.assertEqual((module.RAPL_ENERGY, module.RAPL_MAX), (RAPL % 1 + "energy_uj", RAPL % 1 + "max_energy_range_uj"))

    def test_a_fan_without_a_limit_gives_no_fan_ceiling(self):
        self.fake.write(HWMON % 0 + "fan1_input", "900\n")
        self.fake.write(HWMON % 0 + "fan1_label", "cpu_fan\n")
        self.assertEqual(self.module().CPU_FAN_MAX, 0)
        self.fake.write(HWMON % 0 + "fan1_max", "3000\n")
        self.assertEqual(self.module().CPU_FAN_MAX, 3000)

    def test_a_wrapped_energy_counter_without_a_range_reports_zero_watts(self):
        self.rapl(0, "package-0", energy_uj=99000000)
        module = self.module()
        self.build_at(module, 100.0)
        self.rapl(0, "package-0", energy_uj=1000000)
        self.assertEqual(self.build_at(module, 102.0)["cpu"]["watts"], 0.0)

    def test_the_script_exits_with_the_status_of_its_command(self):
        output = io.StringIO()
        with mock.patch("sys.argv", ["sysmon-collect", "--snapshot"]), contextlib.redirect_stdout(output), \
                self.assertRaises(SystemExit) as raised:
            self.module()
            runpy.run_path(str(SCRIPT), run_name="__main__")
        self.assertEqual(raised.exception.code, 0)
        self.assertIn('"ts":0', output.getvalue())


if __name__ == "__main__":
    unittest.main()
