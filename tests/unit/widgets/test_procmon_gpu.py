#!/usr/bin/env python3
import json
import sys
import types
import unittest
from unittest import mock

from collectorharness import WIDGETS, CollectorTest
from test_procmon_collect import fake_process_nvml

SCRIPT = WIDGETS / "process-monitor" / "bin" / "procmon_gpu.py"


class TestProcmonGpu(CollectorTest):
    def module(self, nvml=None):
        return self.load(SCRIPT, "procmon_gpu_direct", pynvml=nvml)

    def nvml(self, **overrides):
        nvml = fake_process_nvml([], [], [])
        for name, value in overrides.items():
            setattr(nvml, name, value)
        return nvml

    def test_the_video_memory_total_is_read_once_and_remembered(self):
        initialised = []
        nvml = self.nvml(nvmlInit=lambda: initialised.append(1))
        module = self.module(nvml)
        self.assertEqual(module.vram_total(), 8 << 30)
        nvml.nvmlDeviceGetMemoryInfo = mock.Mock(side_effect=AssertionError)
        self.assertEqual(module.vram_total(), 8 << 30)
        self.assertEqual(initialised, [1])

    def test_the_video_memory_total_is_zero_without_a_driver(self):
        self.assertEqual(self.module(None).vram_total(), 0)

    def test_a_failing_driver_init_is_retried_on_the_next_reading(self):
        attempts = []

        def init():
            attempts.append(1)
            if len(attempts) == 1:
                raise RuntimeError("driver not loaded")
        module = self.module(self.nvml(nvmlInit=init))
        self.assertEqual(module.vram_total(), 0)
        self.assertEqual(module.gpu_usage(0.5), {})
        self.assertEqual(module.vram_total(), 8 << 30)
        self.assertEqual(len(attempts), 2)

    def test_usage_is_empty_when_the_driver_fails_while_sampling(self):
        module = self.module(self.nvml(nvmlInit=mock.Mock(side_effect=RuntimeError)))
        self.assertEqual(module.gpu_usage(0.5), {})

    def test_usage_is_empty_without_a_driver(self):
        self.assertEqual(self.module(None).gpu_usage(0.5), {})

    def test_usage_is_per_process_across_compute_and_graphics(self):
        samples = [(10, 100, 30, 0, 5, 0), (10, 200, 40, 0, 6, 7), (11, 150, 10, 0, 0, 0)]
        module = self.module(fake_process_nvml(samples, [(10, 1 << 20)], [(12, 2 << 20)]))
        self.assertEqual(module.gpu_usage(0.5), {
            10: {"gpu": 40, "enc": 6, "dec": 7, "vram": 1 << 20},
            12: {"gpu": 0, "enc": 0, "dec": 0, "vram": 2 << 20},
            11: {"gpu": 10, "enc": 0, "dec": 0, "vram": 0},
        })


class TestFrameRates(CollectorTest):
    def setUp(self):
        super().setUp()
        self.proxy = mock.Mock()
        self.bus = mock.Mock()
        self.bus.get_object.return_value = self.proxy
        self.dbus = types.SimpleNamespace(SessionBus=lambda: self.bus)
        self.module = self.load(SCRIPT, "procmon_gpu_frames")
        self.start(mock.patch.dict(sys.modules, {"dbus": self.dbus}))

    def reply(self, items):
        self.proxy.Frames.return_value = json.dumps(items)

    def test_the_best_reading_per_process_is_reported(self):
        self.reply([{"pid": 7, "fps": 60, "frametime": 16.666, "fps_low": 50}, {"pid": 7, "fps": 90, "frametime": 11.111, "fps_low": 70},
                    {"pid": 7, "fps": 30}, {"pid": 8, "fps": 144}])
        self.assertEqual(self.module.frame_rates(), {7: {"fps": 90, "frametime": 11.11, "fps_low": 70},
                                                     8: {"fps": 144, "frametime": 0.0, "fps_low": 0}})

    def test_entries_without_a_process_or_a_rate_are_skipped(self):
        self.reply([{"pid": 0, "fps": 60}, {"pid": -3, "fps": 60}, {"fps": 60}, {"pid": 9}, {"pid": None, "fps": 1}])
        self.assertEqual(self.module.frame_rates(), {})

    def test_an_empty_reply_reports_nothing(self):
        self.proxy.Frames.return_value = ""
        self.assertEqual(self.module.frame_rates(), {})

    def test_the_bus_object_is_looked_up_once(self):
        self.reply([])
        self.module.frame_rates()
        self.module.frame_rates()
        self.bus.get_object.assert_called_once_with("org.devl0rd.ProcessMonitor.FrameTelemetry", "/FrameTelemetry")
        self.assertEqual(self.proxy.Frames.call_count, 2)

    def test_a_failure_drops_the_object_and_waits_before_retrying(self):
        self.proxy.Frames.side_effect = RuntimeError("service gone")
        with mock.patch.object(self.module.time, "monotonic", return_value=100.0):
            self.assertEqual(self.module.frame_rates(), {})
            self.assertEqual(self.module.frame_rates(), {})
        self.assertEqual(self.bus.get_object.call_count, 1)
        with mock.patch.object(self.module.time, "monotonic", return_value=103.0):
            self.proxy.Frames.side_effect = None
            self.reply([{"pid": 4, "fps": 75}])
            self.assertEqual(self.module.frame_rates(), {4: {"fps": 75, "frametime": 0.0, "fps_low": 0}})
        self.assertEqual(self.bus.get_object.call_count, 2)


if __name__ == "__main__":
    unittest.main()
