#!/usr/bin/env python3
import builtins
import shutil
import unittest
from unittest import mock

from collectorharness import WIDGETS, CollectorTest

SCRIPT = WIDGETS / "system-monitor" / "bin" / "sysmon_amdgpu.py"
WAKING = ("gpu_metrics", "pp_dpm_sclk", "gpu_busy_percent", "temp1_input", "power1_average", "freq1_input", "pwm1")


class TestSysmonAmdgpu(CollectorTest):
    def setUp(self):
        super().setUp()
        self.module = self.load(SCRIPT, "sysmon_amdgpu_direct")

    def dev(self, card="card0"):
        return "/sys/class/drm/%s/device/" % card

    def card(self, card="card0", vram=(1 << 20, 4 << 30), revision=1, status="active", hwmon=5):
        dev = self.dev(card)
        files = {"gpu_busy_percent": "41", "mem_info_vram_used": vram[0], "mem_info_vram_total": vram[1],
                 "mem_info_gtt_used": 1 << 30, "mem_info_gtt_total": 8 << 30,
                 "pp_dpm_sclk": "0: 300Mhz\n1: 2100Mhz *\n", "hwmon/hwmon%d/temp1_input" % hwmon: "61000"}
        if status:
            files["power/runtime_status"] = status
        for name, text in files.items():
            self.fake.write(dev + name, "%s\n" % text)
        (self.home / ("root" + dev + "gpu_metrics")).write_bytes(bytes([0x08, 0x01, revision, 0]))
        return dev

    def opened(self):
        paths, real = [], builtins.open
        self.start(mock.patch("builtins.open", lambda path, *a, **k: (paths.append(path), real(path, *a, **k))[1]))
        return paths

    def test_without_an_amdgpu_card_there_is_no_gpu(self):
        self.assertIsNone(self.module.amd_gpu())

    def test_a_card_that_appears_later_is_picked_up(self):
        self.assertIsNone(self.module.amd_gpu())
        self.card()
        self.assertEqual(self.module.amd_gpu()["util"], 41)

    def test_a_card_that_vanishes_is_replaced_by_the_one_left(self):
        self.card("card0", vram=(0, 8 << 30))
        self.card("card1", vram=(0, 1 << 29))
        self.assertEqual(self.module.amd_gpu()["vram_total"], 8 << 30)
        shutil.rmtree(self.home / "root" / self.dev("card0").lstrip("/"))
        self.assertEqual(self.module.amd_gpu()["vram_total"], 1 << 29)
        shutil.rmtree(self.home / "root" / self.dev("card1").lstrip("/"))
        self.assertIsNone(self.module.amd_gpu())

    def test_a_suspended_card_is_never_woken_and_is_probed_once_awake(self):
        dev = self.card(revision=3, status="suspended")
        self.fake.write(dev + "product_name", "Radeon Test\n")
        opened = self.opened()
        for _ in range(3):
            gpu = self.module.amd_gpu("AMD Ryzen w/ Radeon 860M")
        self.assertEqual((gpu["util"], gpu["temp"], gpu["vram_used"], gpu["clock_max"], gpu["shared"]), (0, 0, 0, 0, False))
        self.assertEqual(gpu["name"], "Radeon Test")
        self.assertFalse([p for p in opened if p.endswith(WAKING)])
        self.fake.write(dev + "power/runtime_status", "active\n")
        gpu = self.module.amd_gpu("AMD Ryzen w/ Radeon 860M")
        self.assertEqual((gpu["util"], gpu["temp"], gpu["clock_max"], gpu["shared"], gpu["name"]),
                         (41, 61.0, 2100, True, "AMD Radeon 860M"))
        self.assertEqual(gpu["vram_total"], (4 << 30) + (8 << 30))
        opened.clear()
        self.module.amd_gpu("AMD Ryzen w/ Radeon 860M")
        self.assertFalse([p for p in opened if p.endswith(("gpu_metrics", "pp_dpm_sclk", "uevent"))])

    def test_a_suspending_card_counts_as_asleep(self):
        self.card(status="suspending")
        self.assertEqual(self.module.amd_gpu()["util"], 0)

    def test_a_card_without_runtime_status_counts_as_awake(self):
        self.card(status=None)
        self.assertEqual(self.module.amd_gpu()["util"], 41)

    def test_the_hwmon_directory_is_found_again_after_its_index_changes(self):
        self.card(hwmon=5)
        self.assertEqual(self.module.amd_gpu()["temp"], 61.0)
        shutil.rmtree(self.home / "root" / (self.dev() + "hwmon").lstrip("/"))
        self.assertEqual(self.module.amd_gpu()["temp"], 0)
        self.fake.write(self.dev() + "hwmon/hwmon9/temp1_input", "70000\n")
        self.assertEqual(self.module.amd_gpu()["temp"], 70.0)

    def test_malformed_and_missing_values_read_as_zero(self):
        dev = self.card(revision=1)
        self.fake.write(dev + "gpu_busy_percent", "busy\n")
        self.fake.write(dev + "mem_info_vram_total", "lots\n")
        self.fake.write(dev + "pp_dpm_sclk", "nonsense\n")
        self.fake.write(dev + "hwmon/hwmon5/pwm1", "10\n")
        self.fake.write(dev + "hwmon/hwmon5/pwm1_max", "0\n")
        gpu = self.module.amd_gpu()
        self.assertEqual((gpu["util"], gpu["vram_pct"], gpu["clock_max"], gpu["fan"], gpu["power"]), (0, 0, 0, 0, 0))

    def test_instantaneous_power_is_the_fallback_for_average_power(self):
        dev = self.card()
        self.fake.write(dev + "hwmon/hwmon5/power1_input", "9500000\n")
        self.assertEqual(self.module.amd_gpu()["power"], 9.5)

    def test_a_truncated_gpu_metrics_file_is_not_an_apu(self):
        dev = self.card()
        (self.home / ("root" + dev + "gpu_metrics")).write_bytes(b"\x08")
        self.assertFalse(self.module.amd_gpu("AMD Ryzen w/ Radeon 860M")["shared"])

    def test_the_name_comes_from_the_hardware_database_without_brackets(self):
        dev = self.card()
        self.fake.write(dev + "uevent", "PCI_SLOT_NAME=0000:03:00.0\n")
        self.fake.write("/run/udev/data/+pci:0000:03:00.0", "E:ID_MODEL_FROM_DATABASE=Radeon Plain\n")
        self.assertEqual(self.module.amd_gpu()["name"], "AMD Radeon Plain")

    def test_an_unknown_card_is_named_generically(self):
        self.card()
        self.assertEqual(self.module.amd_gpu()["name"], "AMD GPU")


if __name__ == "__main__":
    unittest.main()
