#!/usr/bin/env python3
import builtins
import shutil
import sys
import unittest
from unittest import mock

from collectorharness import WIDGETS, CollectorTest, FakeClock

SCRIPT = WIDGETS / "system-monitor" / "bin" / "sysmon_intelgpu.py"
sys.path.insert(0, str(SCRIPT.parent))
GT = "/sys/class/drm/card1/gt/gt0/"
DEV = "/sys/class/drm/card1/device/"


class TestSysmonIntelgpu(CollectorTest):
    def setUp(self):
        super().setUp()
        self.module = self.load(SCRIPT, "sysmon_intelgpu_direct")
        self.clock = FakeClock(float("inf"))
        self.start(self.clock.patch(self.module))

    def card(self, rc6_ms=1000, status="active"):
        for name, text in {"rc6_residency_ms": rc6_ms, "rps_act_freq_mhz": 800, "rps_RP0_freq_mhz": 1300}.items():
            self.fake.write(GT + name, "%s\n" % text)
        if status:
            self.fake.write(DEV + "power/runtime_status", status + "\n")

    def sample(self, rc6_ms, seconds=2):
        self.clock.mono += seconds
        self.fake.write(GT + "rc6_residency_ms", "%s\n" % rc6_ms)
        return self.module.intel_gpu()

    def test_without_an_i915_card_there_is_no_gpu(self):
        self.assertIsNone(self.module.intel_gpu())

    def test_usage_is_the_time_out_of_rc6_since_the_last_sample(self):
        self.card()
        self.assertEqual(self.module.intel_gpu()["util"], 0)
        gpu = self.sample(1500)
        self.assertEqual((gpu["util"], gpu["clock_gr"], gpu["clock_max"], gpu["vram_total"], gpu["shared"]), (75, 800, 1300, 0, True))
        self.assertEqual(self.sample(3500)["util"], 0)
        self.assertEqual(self.sample(3500)["util"], 100)

    def test_a_suspended_gpu_is_never_woken_and_starts_over_once_awake(self):
        self.card(status="suspended")
        paths, real = [], builtins.open
        self.start(mock.patch("builtins.open", lambda path, *a, **k: (paths.append(path), real(path, *a, **k))[1]))
        self.module.intel_gpu()
        gpu = self.sample(1500)
        self.assertEqual((gpu["util"], gpu["clock_gr"], gpu["clock_max"]), (0, 0, 1300))
        self.assertFalse([p for p in paths if p.endswith(("rc6_residency_ms", "rps_act_freq_mhz"))])
        self.fake.write(DEV + "power/runtime_status", "active\n")
        self.assertEqual(self.sample(1500)["util"], 0)
        self.assertEqual(self.sample(2000)["util"], 75)

    def test_a_gpu_without_runtime_status_counts_as_awake(self):
        self.card(status=None)
        self.module.intel_gpu()
        self.assertEqual(self.sample(1000)["util"], 100)

    def test_a_card_that_vanishes_is_dropped(self):
        self.card()
        self.assertIsNotNone(self.module.intel_gpu())
        shutil.rmtree(self.home / "root" / GT.lstrip("/"))
        self.assertIsNone(self.module.intel_gpu())

    def test_the_name_comes_from_the_hardware_database(self):
        self.card()
        self.fake.write(DEV + "uevent", "DRIVER=i915\nPCI_SLOT_NAME=0000:00:02.0\n")
        self.fake.write("/run/udev/data/+pci:0000:00:02.0", "E:ID_MODEL_FROM_DATABASE=TigerLake-LP GT2 [Iris Xe Graphics]\n")
        self.assertEqual(self.module.intel_gpu()["name"], "Intel Iris Xe Graphics")

    def test_an_unknown_gpu_is_named_generically(self):
        self.card()
        self.assertEqual(self.module.intel_gpu()["name"], "Intel Graphics")


if __name__ == "__main__":
    unittest.main()
