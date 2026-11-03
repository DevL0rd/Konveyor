#!/usr/bin/env python3
import json
import unittest

from collectorharness import WIDGETS, CollectorTest, FakeClock, Stop, load_script

SCRIPT = WIDGETS / "shared" / "collectors.py"


class TestSharedCollectors(CollectorTest):
    def setUp(self):
        super().setUp()
        self.module = load_script(SCRIPT, "collectors_direct")
        self.config_file = self.config / "app" / "config.json"

    def test_an_interval_that_is_not_finite_is_refused(self):
        for value in ("inf", "-inf", "nan", "fast", None):
            with self.subTest(value=value):
                self.assertEqual(self.module.set_interval(str(self.config_file), value), 1)
        self.assertFalse(self.config_file.exists())

    def test_an_interval_is_saved_with_the_other_settings(self):
        self.config_file.parent.mkdir(parents=True)
        self.config_file.write_text(json.dumps({"other": 1}))
        self.assertEqual(self.module.set_interval(str(self.config_file), "3"), 0)
        self.assertEqual(json.loads(self.config_file.read_text()), {"other": 1, "poll_interval": 3.0})

    def test_a_failing_tick_is_reported_and_the_loop_carries_on(self):
        clock = FakeClock(until=5)
        ticks, reports = [], []

        def tick(interval):
            ticks.append(interval)
            if len(ticks) == 1:
                raise ValueError("sensor vanished")
        with clock.patch(self.module), self.assertRaises(Stop):
            self.module.serve(str(self.config_file), 1.0, tick, reports.append)
        self.assertGreater(len(ticks), 2)
        self.assertEqual([str(report) for report in reports], ["sensor vanished"])


if __name__ == "__main__":
    unittest.main()
