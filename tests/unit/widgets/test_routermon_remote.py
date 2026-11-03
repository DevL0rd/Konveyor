#!/usr/bin/env python3
import subprocess
import unittest
from unittest import mock

from collectorharness import WIDGETS, CollectorTest
from router_harness import FakeAdGuard, basic

SCRIPT = WIDGETS / "router-monitor" / "bin" / "routermon-collect"


class RouterModuleTest(CollectorTest):
    def setUp(self):
        super().setUp()
        self.module = self.load(SCRIPT, "routermon_collect")
        self.extras = self.module.extras
        self.remote = self.module.remote
        self.log_file = self.home / ".local/state/Linux-Router-Monitor/monitor.log"


class TestRoutermonLog(RouterModuleTest):
    def test_a_long_log_keeps_only_its_last_thousand_lines(self):
        self.log_file.parent.mkdir(parents=True)
        self.log_file.write_text("".join("old line %05d %s\n" % (number, "x" * 90) for number in range(4000)))
        self.extras.log("fresh")
        lines = self.log_file.read_text().splitlines()
        self.assertEqual(len(lines), 1001)
        self.assertIn("old line 03000", lines[0])
        self.assertTrue(lines[-1].endswith("  fresh"))

    def test_a_short_log_is_appended_to(self):
        self.extras.log("one")
        self.extras.log("two")
        self.assertEqual([line.split("  ")[-1] for line in self.log_file.read_text().splitlines()], ["one", "two"])

    def test_an_unwritable_log_never_stops_the_collector(self):
        with mock.patch.object(self.extras.os, "makedirs", side_effect=PermissionError):
            self.extras.log("lost")
        self.assertFalse(self.log_file.exists())


class TestRoutermonLocalFallback(RouterModuleTest):
    def test_a_malformed_route_table_has_no_default_interface(self):
        self.fake.write("/proc/net/route", "Iface\tDestination\n" "eth0\t00000000\tgw\tzz\tx\tx\t1\t0\n")
        self.assertEqual(self.extras._default_interface(), "")

    def test_a_missing_route_table_has_no_default_interface(self):
        self.assertEqual(self.extras._default_interface(), "")

    def test_a_ping_that_cannot_run_counts_as_total_loss(self):
        with mock.patch.object(subprocess, "run", side_effect=FileNotFoundError("ping")):
            snapshot, state = self.extras.local_network_fallback({}, {}, "offline")
        self.assertEqual((snapshot["network"]["ping_rtt"], snapshot["network"]["ping_loss"]), (0, 100))
        self.assertEqual(state["local_fallback"]["ping_loss"], 100)

    def test_a_ping_that_times_out_counts_as_total_loss(self):
        with mock.patch.object(subprocess, "run", side_effect=subprocess.TimeoutExpired("ping", 3)):
            snapshot, _ = self.extras.local_network_fallback({}, {}, "offline")
        self.assertEqual(snapshot["network"]["ping_loss"], 100)


class TestRoutermonAdGuard(RouterModuleTest):
    def setUp(self):
        super().setUp()
        self.server = FakeAdGuard()
        self.addCleanup(self.server.close)
        self.cfg = {"adguard": {"enabled": True, "url": self.server.url, "username": "user", "password": "secret"}}

    def test_stats_and_status_are_fetched_with_the_saved_login(self):
        self.server.reply("/control/stats", {"num_dns_queries": 7})
        self.server.reply("/control/status", {"protection_enabled": True})
        self.assertEqual(self.extras.fetch_agh(self.cfg), {"stats": {"num_dns_queries": 7}, "status": {"protection_enabled": True}})
        self.assertEqual([(request["path"], request["authorization"]) for request in self.server.requests],
                         [("/control/stats", basic("user", "secret")), ("/control/status", basic("user", "secret"))])

    def test_no_login_is_sent_without_a_username(self):
        self.cfg["adguard"].pop("username")
        self.server.reply("/control/stats", {})
        self.server.reply("/control/status", {})
        self.extras.fetch_agh(self.cfg)
        self.assertEqual({request["authorization"] for request in self.server.requests}, {None})

    def test_an_answer_that_is_not_json_is_logged_and_left_out(self):
        self.server.reply("/control/stats", {"num_dns_queries": 1})
        self.server.reply("/control/status", "<html>login</html>")
        self.assertEqual(self.extras.fetch_agh(self.cfg), {"stats": {"num_dns_queries": 1}, "status": None})
        self.assertIn("AGH /control/status failed", self.log_file.read_text())

    def test_without_a_url_nothing_is_fetched(self):
        self.cfg["adguard"]["url"] = ""
        self.assertIsNone(self.extras.fetch_agh(self.cfg))
        self.assertEqual(self.server.requests, [])


class TestRoutermonSessionHandling(RouterModuleTest):
    def setUp(self):
        super().setUp()
        self.addCleanup(self.remote._session.update, proc=None, cmd=None)
        self.cfg = {"host": "router"}

    def process(self, polled=None):
        process = mock.Mock()
        process.poll.return_value = polled
        return process

    def test_a_session_that_will_not_die_is_forgotten_anyway(self):
        stuck = self.process()
        stuck.kill.side_effect = OSError("no such process")
        self.remote._session["proc"] = stuck
        self.remote._close_session()
        self.assertIsNone(self.remote._session["proc"])

    def test_a_live_session_is_reused_and_an_exited_or_reconfigured_one_replaced(self):
        first, second, third = self.process(), self.process(), self.process()
        with mock.patch.object(subprocess, "Popen", side_effect=[first, second, third]) as popen:
            self.assertIs(self.remote._session_for(self.cfg), first)
            self.assertIs(self.remote._session_for(self.cfg), first)
            first.poll.return_value = 1
            self.assertIs(self.remote._session_for(self.cfg), second)
            self.assertIs(self.remote._session_for({**self.cfg, "remote_script": "/other.sh"}), third)
        self.assertEqual(popen.call_count, 3)
        first.kill.assert_called_once_with()
        second.kill.assert_called_once_with()

    def test_blank_lines_in_the_router_output_are_ignored(self):
        self.assertEqual(self.remote.parse_remote("\nwan_ip=1.2.3.4\n\nEND=1\n")["wan_ip"], "1.2.3.4")

    def test_a_ping_that_cannot_run_reports_nothing(self):
        with mock.patch.object(subprocess, "run", side_effect=FileNotFoundError("ssh")):
            self.assertIsNone(self.remote.compute_ping(self.cfg))

    def test_a_ping_without_a_summary_reports_nothing(self):
        with mock.patch.object(subprocess, "run", return_value=mock.Mock(stdout="ping: bad address\n")):
            self.assertIsNone(self.remote.compute_ping(self.cfg))

    def test_a_ping_summary_without_a_loss_line_counts_as_no_loss(self):
        with mock.patch.object(subprocess, "run", return_value=mock.Mock(stdout="round-trip min/avg/max = 1.0/2.5/3.0 ms\n")):
            self.assertEqual(self.remote.compute_ping(self.cfg), {"rtt": 2.5, "loss": 0})


if __name__ == "__main__":
    unittest.main()
