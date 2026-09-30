#!/usr/bin/env python3
import contextlib
import io
import json
import os
import unittest
from unittest import mock

from collectorharness import RECORDING_STUB, WIDGETS, CollectorTest, FakeClock, Stop, write_stub

SCRIPT = WIDGETS / "router-monitor" / "bin" / "routermon-collect"
UNREACHABLE = RECORDING_STUB + """sys.stderr.write("ssh: connect to host router port 22: No route to host\\n")
sys.exit(255)
"""
PING = RECORDING_STUB + """print("64 bytes from 8.8.8.8: icmp_seq=1 ttl=117 time=12.4 ms")
"""
ROUTER_PING = RECORDING_STUB + """print("3 packets transmitted, 3 received, 0% packet loss")
print("round-trip min/avg/max = 10.0/20.5/30.0 ms")
"""
REMOTE = """TS=1000
load1=0.10
load5=0.20
load15=0.30
procs=1/100
uptime=5000.5
cores=2
STAT cpu 100 0 100 700 100 0 0
STAT cpu0 50 0 50 350 50 0 0
STAT cpu1 50 0 50 350 50 0 0
mem_total=1000
mem_avail=250
swap_total=0
swap_free=0
cpu_temp_milli=55500
conntrack=10
conntrack_max=100
wan_ifname=ppp0
IF ppp0 1000000 2000000
IF br0 500 700
STA mac=AA:BB:CC:00:00:01 band=5GHz-1 phy=1200 data_mbps=8
RCOUNT band=5GHz-1 clients=1
RADIO band=5GHz-1 chan=36 width=160 noise=-90 temp=50 busy=12 glitch=1 badplcp=2 dfs=0
STASLOW mac=aa:bb:cc:00:00:01 rssi=-55 txrate=866000 rxrate=433000
LEASE mac=aa:bb:cc:00:00:01 ip=192.168.1.10 name=laptop
LEASE mac=aa:bb:cc:00:00:02 ip=192.168.1.11 name=*
ARP aa:bb:cc:00:00:02 0x2
ARP aa:bb:cc:00:00:03 0x0
BLOCKED AA:BB:CC:00:00:02
wan_proto=pppoe
wan_ip=203.0.113.5
wan_state=2
wrs_protect=1
wrs_mals=0
ping_rtt=11.26
ping_loss=0
END=1
"""
STATIC = """model=RT-AX
fw=388.1
lan_ip=192.168.1.1
RSTATIC band=5GHz-1 ssid=Home Net radio=1 txpower=100
PORT LAN1 Up 1000
PORT LAN2 Down
END=1
"""


class TestRoutermonParsing(CollectorTest):
    def setUp(self):
        super().setUp()
        self.module = self.load(SCRIPT, "routermon_collect")

    def test_parses_every_record_kind(self):
        parsed = self.module.remote.parse_remote(REMOTE + STATIC)
        self.assertEqual(parsed["_stat"]["cpu0"], [50, 0, 50, 350, 50, 0, 0])
        self.assertEqual(parsed["ifaces"]["ppp0"], {"rx": 1000000, "tx": 2000000})
        self.assertEqual(parsed["radios_static"][0]["ssid"], "Home Net")
        self.assertEqual(parsed["ports"], [{"port": "LAN1", "link": "Up 1000"}, {"port": "LAN2", "link": "Down"}])
        self.assertEqual(parsed["arp"][1], {"mac": "aa:bb:cc:00:00:03", "flags": "0x0"})
        self.assertEqual(parsed["blocked"], ["AA:BB:CC:00:00:02"])
        self.assertEqual((parsed["wan_proto"], parsed["END"]), ("pppoe", "1"))

    def test_helpers(self):
        self.assertEqual(self.module.extras._toplist([{"a.com": 5}, {}, "junk", {"b.com": 2}], 3), [{"name": "a.com", "count": 5}])
        self.assertEqual(self.module._num("x", int, 7), 7)
        self.assertEqual(self.module.rate(10, 20, 1), 0.0)
        self.assertEqual(self.module.rate(30, 10, 2), 10.0)
        self.assertEqual(self.module.cpu_util({"cpu": [10, 0, 0, 10, 0]}, {"cpu": [0, 0, 0, 0, 0]}), {"cores": [], "total": 50.0})

    def test_ssh_uses_the_configured_login_and_shared_socket(self):
        command = self.module.remote.ssh_base({"host": "router", "user": "root", "ssh_key": "~/.ssh/router", "control_persist": "30"})
        self.assertEqual(command[-1], "root@router")
        self.assertIn(str(self.home / ".ssh" / "router"), command)
        self.assertIn("ControlPath=%s" % (self.runtime / "Linux-Router-Monitor" / "cm.sock"), command)
        self.assertIn("ControlPersist=30", command)
        self.assertIn("BatchMode=yes", command)

SESSION = RECORDING_STUB + """import time
with open(os.environ["STUB_LOG"]) as log:
    session = sum(1 for line in log if line.startswith('["ssh"')) - 1
replies = iter(os.environ["ROUTER_REPLIES"].split("|")[session].split(","))
for line in sys.stdin:
    reply = next(replies, "exit")
    if reply == "exit":
        sys.exit(255)
    if reply == "hang":
        time.sleep(30)
    sys.stdout.write(("END=1\\n" if reply == "ok" else "partial=1\\n") + "__LRM_DONE__\\n")
    sys.stdout.flush()
"""


class TestRoutermonSession(CollectorTest):
    def setUp(self):
        super().setUp()
        write_stub(self.stubs, "ssh", SESSION)
        self.module = self.load(SCRIPT, "routermon_collect")
        self.module.remote.SESSION_TIMEOUT = 1
        self.addCleanup(self.module.remote._close_session)
        self.cfg = {"host": "router"}

    def outcomes(self, replies, requests):
        os.environ["ROUTER_REPLIES"] = replies
        results = []
        for _ in range(requests):
            try:
                results.append(self.module.remote.fetch_remote(self.cfg).strip())
            except RuntimeError as error:
                results.append(str(error))
        return results, len(self.calls())

    def test_one_ssh_session_serves_every_poll(self):
        self.assertEqual(self.outcomes("ok,ok,ok", 3), (["END=1"] * 3, 1))

    def test_a_dropped_session_is_reported_and_the_next_poll_reconnects(self):
        results, sessions = self.outcomes("ok,exit|ok", 3)
        self.assertEqual(results[0], "END=1")
        self.assertIn("remote session ended", results[1])
        self.assertEqual((results[2], sessions), ("END=1", 2))

    def test_a_hung_router_times_out_and_the_next_poll_reconnects(self):
        results, sessions = self.outcomes("hang|ok", 2)
        self.assertEqual(results[0], "remote collect failed: remote collect timed out")
        self.assertEqual((results[1], sessions), ("END=1", 2))

    def test_output_without_the_end_marker_drops_the_session(self):
        results, sessions = self.outcomes("partial|ok", 2)
        self.assertEqual(results[0], "remote collect failed: incomplete output")
        self.assertEqual((results[1], sessions), ("END=1", 2))


class TestRoutermonBuild(CollectorTest):
    def setUp(self):
        super().setUp()
        write_stub(self.stubs, "ssh", ROUTER_PING)
        self.module = self.load(SCRIPT, "routermon_collect")
        self.cfg = {"host": "router", "slow_every": 10, "adguard": {"url": "http://agh"}}

    def build(self, previous, remote=REMOTE):
        with mock.patch.object(self.module.remote, "fetch_remote", return_value=remote) as fetch:
            snapshot, state = self.module.build(self.cfg, previous)
        return snapshot, state, fetch

    def test_the_first_poll_fetches_everything(self):
        with mock.patch.object(self.module.remote, "fetch_remote", return_value=STATIC):
            previous = {"static_cache": self.module.remote.compute_static(self.cfg)}
        snapshot, state, fetch = self.build(previous)
        self.assertEqual(fetch.call_args.args[1:], (2, True))
        self.assertEqual(snapshot["info"]["admin_url"], "http://192.168.1.1")
        self.assertEqual(snapshot["info"]["agh_url"], "http://agh")
        self.assertEqual(snapshot["network"]["ports"], [{"port": "LAN1", "link": "Up 1000"}])
        self.assertTrue(snapshot["network"]["wan_up"])
        self.assertEqual((snapshot["network"]["ping_rtt"], snapshot["network"]["ping_loss"]), (11.3, 0))
        self.assertEqual(snapshot["system"]["mem_used_pct"], 75.0)
        self.assertEqual(snapshot["system"]["cpu_temp"], 55.5)
        self.assertEqual(snapshot["system"]["swap_used_pct"], 0.0)
        radio = snapshot["wifi"]["radios"][0]
        self.assertEqual((radio["ssid"], radio["on"], radio["clients"], radio["chan"]), ("Home Net", True, 1, 36))
        station = snapshot["wifi"]["stations"][0]
        self.assertEqual((station["rssi"], station["tx_mbps"], station["traffic_bps"]), (-55, 866.0, 1000000.0))
        leases = {lease["ip"]: lease for lease in snapshot["clients"]["leases"]}
        self.assertTrue(leases["192.168.1.10"]["connected"])
        self.assertEqual(leases["192.168.1.10"]["traffic_bps"], 1000000.0)
        self.assertEqual(leases["192.168.1.11"]["name"], "192.168.1.11")
        self.assertTrue(leases["192.168.1.11"]["connected"] and leases["192.168.1.11"]["blocked"])
        self.assertTrue(snapshot["security"]["wrs_protect"])
        self.assertEqual(state["tick"], 1)

    def test_rates_come_from_the_previous_poll(self):
        _, state, _ = self.build({})
        later = REMOTE.replace("TS=1000", "TS=1002").replace("IF ppp0 1000000 2000000", "IF ppp0 3500000 2500000")
        later = later.replace("STAT cpu 100 0 100 700 100 0 0", "STAT cpu 200 0 200 800 200 0 0")
        snapshot, state, fetch = self.build(state, later)
        self.assertEqual(fetch.call_args.args[1:], (3, False))
        self.assertEqual((snapshot["network"]["down_mbps"], snapshot["network"]["up_mbps"]), (10.0, 2.0))
        self.assertEqual(snapshot["system"]["cpu"]["total"], 50.0)
        self.assertEqual(snapshot["clients"]["count"], 2)
        self.assertEqual(state["tick"], 2)

    def test_a_router_ping_is_averaged(self):
        self.assertEqual(self.module.remote.compute_ping(self.cfg), {"rtt": 20.5, "loss": 0})

    def test_adguard_stats_become_the_dns_block(self):
        stats = {"num_dns_queries": 200, "num_blocked_filtering": 50, "avg_processing_time": 0.0123,
                 "top_clients": [{"192.168.1.10": 9}], "top_queried_domains": [{"a.com": 3}], "dns_queries": [1, 2]}
        agh = {"stats": stats, "status": {"protection_enabled": True}}
        with mock.patch.object(self.module.extras, "fetch_agh", return_value=agh):
            dns, previous = self.module.extras.compute_dns(self.cfg, {"q": 100, "ts": 90.0}, [{"ip": "192.168.1.10", "name": "laptop"}], 100.0)
        self.assertEqual((dns["blocked_pct"], dns["qps"], dns["avg_ms"], dns["protection"]), (25.0, 10.0, 12.3, True))
        self.assertEqual(dns["top_clients"], [{"name": "laptop", "count": 9}])
        self.assertEqual(previous, {"q": 200, "ts": 100.0})

    def test_adguard_off_or_unreachable_keeps_the_previous_counters(self):
        self.assertIsNone(self.module.extras.fetch_agh({"adguard": {"enabled": False, "url": "http://agh"}}))
        cfg = {"adguard": {"enabled": True, "url": "http://127.0.0.1:9", "username": "u", "password": "p"}}
        self.assertEqual(self.module.extras.fetch_agh(cfg), {"stats": None, "status": None})
        self.assertEqual(self.module.extras.compute_dns(cfg, {"q": 1}, [], 5.0), (None, {"q": 1}))
        self.assertIn("AGH /control/stats failed", (self.home / ".local/state/Linux-Router-Monitor/monitor.log").read_text())


class TestRoutermonOffline(CollectorTest):
    def setUp(self):
        super().setUp()
        write_stub(self.stubs, "ping", PING)
        write_stub(self.stubs, "ssh", UNREACHABLE)
        self.fake.write("/proc/net/route", "Iface\tDestination\tGateway\tFlags\tRefCnt\tUse\tMetric\tMask\n"
                        "wlan0\t00000000\t0101A8C0\t0003\t0\t0\t600\t00000000\n"
                        "eth0\t00000000\t0101A8C0\t0003\t0\t0\t100\t00000000\n"
                        "eth0\t0001A8C0\t00000000\t0001\t0\t0\t100\t00FFFFFF\n")
        self.fake.write("/proc/net/dev", "Inter-|\n face |\n  eth0: 1000 0 0 0 0 0 0 0 2000 0 0 0 0 0 0 0\n")
        self.module = self.load(SCRIPT, "routermon_collect")
        (self.config / "Linux-Router-Monitor").mkdir(parents=True)

    def configure(self, **cfg):
        (self.config / "Linux-Router-Monitor" / "config.json").write_text(json.dumps(cfg))

    def run_main(self, *arguments):
        output = io.StringIO()
        with mock.patch.object(self.module.sys, "argv", ["routermon-collect", *arguments]), contextlib.redirect_stdout(output):
            self.module.main()
        return json.loads(output.getvalue())

    def test_without_a_router_it_measures_the_local_link(self):
        self.configure(host="", ping_target="9.9.9.9")
        snapshot = self.run_main()
        self.assertEqual(snapshot["fallback"], "local")
        self.assertEqual(snapshot["error"], "Router not set up yet")
        self.assertEqual((snapshot["network"]["ping_rtt"], snapshot["network"]["ping_loss"]), (12.4, 0))
        self.assertEqual(self.calls(), [["ping", "-n", "-c", "1", "-W", "1", "9.9.9.9"]])
        state = json.loads((self.runtime / "Linux-Router-Monitor" / "state.json").read_text())
        self.assertEqual((state["local_fallback"]["interface"], state["local_fallback"]["rx"]), ("eth0", 1000))

    def test_an_unreachable_router_reports_the_error_and_logs_it(self):
        self.configure(host="router")
        snapshot = self.run_main("network")
        self.assertEqual(snapshot["ping_rtt"], 12.4)
        cache = json.loads((self.runtime / "Linux-Router-Monitor" / "data.json").read_text())
        self.assertFalse(cache["online"])
        self.assertIn("remote collect failed", cache["error"])
        self.assertIn("refresh error: remote collect failed", (self.home / ".local/state/Linux-Router-Monitor/monitor.log").read_text())
        self.assertEqual(self.calls()[0][0], "ssh")

    def test_a_fresh_cache_is_served_without_contacting_anything(self):
        self.configure(host="router", cache_ttl=60)
        (self.runtime / "Linux-Router-Monitor").mkdir()
        (self.runtime / "Linux-Router-Monitor" / "data.json").write_text(json.dumps({"online": True, "system": {"cores": 4}}))
        self.assertEqual(self.run_main("system"), {"cores": 4})
        self.assertEqual(self.run_main("nothing"), {"online": True, "system": {"cores": 4}})
        self.assertEqual(self.run_main("all"), self.run_main())
        self.assertEqual(self.calls(), [])

    def test_pausing_serves_the_last_snapshot_flagged_paused(self):
        (self.runtime / "Linux-Router-Monitor").mkdir()
        (self.runtime / "Linux-Router-Monitor" / "paused").write_text("")
        self.assertEqual(self.run_main(), {"online": True, "paused": True})
        self.assertEqual(self.calls(), [])

    def test_a_missing_config_fails_loudly(self):
        with self.assertRaises(FileNotFoundError):
            self.run_main()


class TestRoutermonServe(CollectorTest):
    def setUp(self):
        super().setUp()
        write_stub(self.stubs, "ping", PING)
        self.fake.write("/proc/net/route", "Iface\tDestination\tGateway\tFlags\tRefCnt\tUse\tMetric\tMask\n")
        self.module = self.load(SCRIPT, "routermon_collect")
        (self.config / "Linux-Router-Monitor").mkdir(parents=True)
        self.reachable = []
        self.fetches = []
        self.cache = self.runtime / "Linux-Router-Monitor" / "data.json"
        self.log_file = self.home / ".local/state/Linux-Router-Monitor/monitor.log"

    def configure(self, text):
        (self.config / "Linux-Router-Monitor" / "config.json").write_text(text)

    def fetch_remote(self, cfg, do_ping=True, do_slow=True, do_static=False):
        self.fetches.append("static" if do_static else "poll")
        if self.reachable and not self.reachable.pop(0):
            raise RuntimeError("remote collect failed: ssh: connect to host router port 22: No route to host")
        return STATIC if do_static else REMOTE

    def serve(self, until, on_sleep=None):
        caches = []
        clock = self.clock = FakeClock(until)

        def sleep(seconds):
            caches.append(json.loads(self.cache.read_text()))
            if on_sleep:
                on_sleep(len(caches))
            FakeClock.sleep(clock, seconds)
        clock.sleep = sleep
        with clock.patch(self.module), clock.patch(self.module.extras), \
                mock.patch.object(self.module.remote, "fetch_remote", side_effect=self.fetch_remote), \
                self.assertRaises(Stop):
            self.module.serve()
        return caches

    def test_the_router_identity_is_fetched_again_after_the_router_comes_back(self):
        self.configure(json.dumps({"host": "router", "poll_interval": 1}))
        self.reachable = [True, True, False, True, False, True]
        caches = self.serve(until=2.5)
        self.assertEqual([cache["online"] for cache in caches], [True, False, True])
        self.assertIn("No route to host", caches[1]["error"])
        self.assertEqual(caches[2]["info"]["model"], "RT-AX")
        self.assertEqual(caches[2]["wifi"]["radios"][0]["ssid"], "Home Net")
        self.assertEqual(self.fetches, ["poll", "static", "poll", "poll", "static"])

    def test_a_bad_interval_setting_is_logged_once_and_the_default_used(self):
        for value in ("fast", None, "inf", 0, -5):
            with self.subTest(value=value):
                self.log_file.unlink(missing_ok=True)
                self.configure(json.dumps({"host": "", "poll_interval": value, "slow_every": "often"}))
                caches = self.serve(until=10)
                self.assertGreaterEqual(len(caches), 10 / (0.25 if value in (0, -5) else 0.5) - 1)
                self.assertLessEqual(len(caches), 10 / 0.25 + 1)
                if value not in (0, -5):
                    self.assertEqual(self.log_file.read_text().count("poll_interval"), 1)
                self.assertEqual(self.log_file.read_text().count("slow_every"), 1)

    def test_the_local_ping_keeps_running_after_the_clock_is_set_back(self):
        self.configure(json.dumps({"host": "", "poll_interval": 1}))

        def step_back(count):
            if count == 2:
                self.clock.wall -= 3600
        self.serve(until=20, on_sleep=step_back)
        self.assertGreaterEqual(len([call for call in self.calls() if call[0] == "ping"]), 4)

    def test_an_unreadable_config_is_shown_until_it_is_fixed(self):
        self.configure('{"host": "router",')

        def fix(count):
            if count == 2:
                self.configure(json.dumps({"host": "router"}))
        caches = self.serve(until=6, on_sleep=fix)
        self.assertFalse(caches[0]["online"])
        self.assertIn("config.json", caches[0]["error"])
        self.assertTrue(caches[-1]["online"])


if __name__ == "__main__":
    unittest.main()
