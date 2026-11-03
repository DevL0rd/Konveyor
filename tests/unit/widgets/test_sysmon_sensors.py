#!/usr/bin/env python3
import os
import unittest
from unittest import mock

from collectorharness import WIDGETS, CollectorTest

SCRIPT = WIDGETS / "system-monitor" / "bin" / "sysmon_sensors.py"
HWMON = "/sys/class/hwmon/hwmon%d/"


class TestSysmonSensors(CollectorTest):
    def setUp(self):
        super().setUp()
        self.module = self.load(SCRIPT, "sysmon_sensors_direct")
        self.addCleanup(self.module._close_sensors)

    def temperature(self, hwmon, index, value, label=None, name="coretemp"):
        self.fake.write(HWMON % hwmon + "name", name + "\n")
        self.fake.write(HWMON % hwmon + "temp%d_input" % index, "%s\n" % value)
        if label is not None:
            self.fake.write(HWMON % hwmon + "temp%d_label" % index, label + "\n")

    def frequency(self, cpu, value):
        return self.fake.write("/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq" % cpu, "%s\n" % value)

    def test_frequencies_are_in_megahertz_by_cpu_number(self):
        self.frequency(0, 4000000)
        self.frequency(12, 2500000)
        self.assertEqual(self.module.cpu_freqs(), {0: 4000.0, 12: 2500.0})

    def test_an_unopenable_frequency_sensor_is_left_out(self):
        self.frequency(0, 4000000)
        self.frequency(1, 3000000).chmod(0)
        self.assertEqual(self.module.cpu_freqs(), {0: 4000.0})

    def test_an_unreadable_frequency_triggers_a_rescan(self):
        path = self.frequency(0, 4000000)
        self.assertEqual(self.module.cpu_freqs(), {0: 4000.0})
        path.write_text("<unknown>\n")
        self.assertEqual(self.module.cpu_freqs(), {})
        self.assertIsNone(self.module._sensors["when"])
        path.write_text("1000000\n")
        self.assertEqual(self.module.cpu_freqs(), {0: 1000.0})

    def test_per_core_sensors_are_averaged(self):
        self.temperature(0, 1, 90000, "Package id 0")
        self.temperature(0, 2, 50000, "Core 0")
        self.temperature(0, 3, 71000, "Core 1")
        self.assertEqual(self.module.cpu_temp(), 60)

    def test_the_package_sensor_is_preferred_over_the_die_sensors(self):
        self.temperature(0, 1, 80000, "Package id 0")
        self.temperature(0, 2, 70000, "Tdie", name="k10temp")
        self.assertEqual(self.module.cpu_temp(), 80)

    def test_the_die_sensor_is_preferred_over_the_control_sensor(self):
        self.temperature(0, 1, 70000, "Tctl", name="k10temp")
        self.temperature(0, 2, 65000, "Tdie", name="k10temp")
        self.assertEqual(self.module.cpu_temp(), 65)

    def test_the_control_sensor_alone_is_used_when_nothing_better_exists(self):
        self.temperature(0, 1, 72000, "Tctl", name="k10temp")
        self.assertEqual(self.module.cpu_temp(), 72)

    def test_sensors_without_labels_count_as_cores(self):
        self.temperature(0, 1, 40000)
        self.temperature(0, 2, 60000)
        self.assertEqual(self.module.cpu_temp(), 50)

    def test_other_hardware_monitors_are_ignored(self):
        self.temperature(0, 1, 99000, "Core 0", name="nvme")
        self.fake.write(HWMON % 1 + "temp1_input", "50000\n")
        self.assertEqual(self.module.cpu_temp(), 0)

    def test_an_unopenable_temperature_sensor_is_left_out(self):
        self.temperature(0, 1, 40000, "Core 0")
        self.temperature(0, 2, 60000, "Core 1")
        (self.home / ("root" + HWMON % 0 + "temp2_input")).chmod(0)
        self.assertEqual(self.module.cpu_temp(), 40)

    def test_an_unreadable_temperature_is_skipped_and_triggers_a_rescan(self):
        self.temperature(0, 1, 40000, "Core 0")
        self.temperature(0, 2, "garbage", "Core 1")
        self.assertEqual(self.module.cpu_temp(), 40)
        self.assertIsNone(self.module._sensors["when"])

    def test_no_sensors_reads_zero(self):
        self.assertEqual(self.module.cpu_temp(), 0)

    def fan(self, hwmon, value, label=None):
        self.fake.write(HWMON % hwmon + "name", "board\n")
        self.fake.write(HWMON % hwmon + "fan1_input", "%s\n" % value)
        if label is not None:
            self.fake.write(HWMON % hwmon + "fan1_label", label + "\n")

    def test_the_cpu_fan_is_found_by_its_label(self):
        self.fan(0, 900, "chassis")
        self.fan(1, 1500, "CPU Fan")
        self.assertEqual(self.module.cpu_fan(), 1500)

    def test_fans_without_a_label_are_not_the_cpu_fan(self):
        self.fan(0, 900)
        self.assertIsNone(self.module.cpu_fan())

    def test_an_unreadable_fan_reports_nothing_and_triggers_a_rescan(self):
        self.fan(0, "garbage", "cpu")
        self.assertIsNone(self.module.cpu_fan())
        self.assertIsNone(self.module._sensors["when"])

    def test_a_cpu_fan_whose_sensor_cannot_be_opened_reports_nothing(self):
        self.fan(0, 1500, "cpu")
        (self.home / ("root" + HWMON % 0 + "fan1_input")).chmod(0)
        self.assertIsNone(self.module.cpu_fan())

    def test_sensors_are_rescanned_after_a_minute(self):
        self.frequency(0, 4000000)
        with mock.patch.object(self.module.time, "monotonic", return_value=100.0):
            self.assertEqual(self.module.cpu_freqs(), {0: 4000.0})
        self.frequency(1, 2000000)
        with mock.patch.object(self.module.time, "monotonic", return_value=130.0):
            self.assertEqual(self.module.cpu_freqs(), {0: 4000.0})
        with mock.patch.object(self.module.time, "monotonic", return_value=161.0):
            self.assertEqual(self.module.cpu_freqs(), {0: 4000.0, 1: 2000.0})

    def test_rescanning_closes_the_previous_descriptors(self):
        self.frequency(0, 4000000)
        self.module.cpu_freqs()
        before = [fd for _, fd in self.module._sensors["freq"]]
        self.module._sensors["when"] = None
        with mock.patch.object(self.module.os, "close", wraps=os.close) as close:
            self.module.cpu_freqs()
        self.assertEqual([call.args[0] for call in close.call_args_list], before)


if __name__ == "__main__":
    unittest.main()
