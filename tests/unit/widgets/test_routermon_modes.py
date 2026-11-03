#!/usr/bin/env python3
import fcntl
import json
import os
import runpy
import unittest
from unittest import mock

from collectorharness import RECORDING_STUB, write_stub
from test_routermon_collect import REMOTE, SCRIPT, STATIC, RouterOfflineBase, RouterServeBase

APP = "Linux-Router-Monitor"


def fetch(cfg, do_ping=True, do_slow=True, do_static=False):
    return STATIC if do_static else REMOTE


class TestRoutermonRefresh(RouterOfflineBase):
    def test_a_successful_refresh_adds_the_dns_block_and_the_router_ping(self):
        self.configure(host="router")
        dns = {"queries_total": 5}
        with mock.patch.object(self.module.remote, "fetch_remote", side_effect=fetch), \
                mock.patch.object(self.module.extras, "compute_dns", return_value=(dns, {"q": 5, "ts": 1.0})), \
                mock.patch.object(self.module.remote, "compute_ping", return_value={"rtt": 9.5, "loss": 2}):
            snapshot = self.run_main()
        self.assertEqual(snapshot["dns"], dns)
        self.assertEqual((snapshot["network"]["ping_rtt"], snapshot["network"]["ping_loss"]), (9.5, 2))
        self.assertEqual(snapshot["info"]["model"], "RT-AX")
        state = json.loads((self.runtime / APP / "state.json").read_text())
        self.assertEqual((state["dns_cache"], state["agh_prev"], state["ping"]), (dns, {"q": 5, "ts": 1.0}, {"rtt": 9.5, "loss": 2}))

    def test_a_damaged_state_file_is_replaced(self):
        self.configure(host="")
        (self.runtime / APP).mkdir()
        (self.runtime / APP / "state.json").write_text("{damaged")
        self.assertEqual(self.run_main()["fallback"], "local")
        self.assertIn("local_fallback", json.loads((self.runtime / APP / "state.json").read_text()))

    def test_an_unreadable_fresh_cache_reports_no_data(self):
        self.configure(host="router", cache_ttl=60)
        (self.runtime / APP).mkdir()
        (self.runtime / APP / "data.json").write_text("{damaged")
        self.assertEqual(self.run_main(), {"online": False, "error": "no data"})
        self.assertEqual(self.calls(), [])

    def test_a_config_that_is_not_an_object_is_rejected(self):
        (self.config / APP / "config.json").write_text("[1, 2]")
        with self.assertRaisesRegex(ValueError, "does not hold a JSON object"):
            self.module.load_config()

    def test_the_config_stamp_changes_with_the_file_and_is_none_without_one(self):
        self.assertIsNone(self.module.config_stamp())
        self.configure(host="router")
        os.utime(self.module.CONFIG, ns=(5, 7))
        self.assertEqual(self.module.config_stamp(), 7)

    def test_the_serve_flag_starts_the_resident_collector(self):
        with mock.patch.object(self.module, "serve") as serve, mock.patch.object(self.module.sys, "argv", ["routermon-collect", "--serve"]):
            self.assertIsNone(self.module.main())
        serve.assert_called_once_with()

    def test_the_script_exits_with_the_status_of_main(self):
        write_stub(self.stubs, "systemctl", RECORDING_STUB)
        with mock.patch("sys.argv", ["routermon-collect", "--restart"]), self.assertRaises(SystemExit) as raised:
            runpy.run_path(str(SCRIPT), run_name="__main__")
        self.assertEqual(raised.exception.code, 0)
        self.assertEqual(self.calls(), [["systemctl", "--user", "start", "konveyor-widgets.service"]])


class TestRoutermonServeModes(RouterServeBase):
    def test_a_second_server_leaves_the_running_one_alone(self):
        rundir = self.runtime / APP
        rundir.mkdir()
        with open(rundir / "serve.lock", "w") as held:
            fcntl.flock(held, fcntl.LOCK_EX)
            self.assertIsNone(self.module.serve())
        self.assertFalse((rundir / "serve.pid").exists())
        self.assertFalse(self.cache.exists())

    def test_pausing_flags_the_last_snapshot_without_touching_the_router(self):
        self.configure(json.dumps({"host": "router"}))
        (self.runtime / APP).mkdir()
        (self.runtime / APP / "paused").write_text("")
        self.assertEqual(self.serve(until=1)[0], {"online": True, "paused": True})
        self.cache.write_text(json.dumps({"online": False, "error": "x"}))
        self.assertEqual(self.serve(until=1)[0], {"online": False, "error": "x", "paused": True})
        self.assertEqual(self.fetches, [])

    def test_the_dns_block_is_added_on_the_slow_tick(self):
        self.configure(json.dumps({"host": "router", "poll_interval": 1, "slow_every": 2}))
        dns = {"queries_total": 9}
        with mock.patch.object(self.module.extras, "compute_dns", return_value=(dns, {"q": 9, "ts": 1.0})) as compute:
            caches = self.serve(until=3)
        self.assertEqual(caches[-1]["dns"], dns)
        self.assertTrue(compute.called)

    def test_a_config_that_is_not_an_object_is_shown_until_it_is_fixed(self):
        self.configure("[1]")
        cache = self.serve(until=1)[0]
        self.assertFalse(cache["online"])
        self.assertIn("does not hold a JSON object", cache["error"])


if __name__ == "__main__":
    unittest.main()
