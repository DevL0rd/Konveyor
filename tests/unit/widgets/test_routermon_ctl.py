#!/usr/bin/env python3
import contextlib
import io
import json
import subprocess
import sys
import unittest
from unittest import mock

from portal_harness import load_script
from router_harness import ROUTER_BIN, FakeAdGuard, RouterTest, basic


SCRIPT = ROUTER_BIN / "routermon-ctl"
CLIENTS = "Laptop>AA:BB:CC:00:00:01>0>0>>>>><TV>AA:BB:CC:00:00:02>0>0>>>>>"
STATIC = "<AA:BB:CC:00:00:01>192.168.1.10>><AA:BB:CC:00:00:02>192.168.1.11>>"


class CtlTest(RouterTest):
    def ctl(self, *arguments):
        result = self.invoke(SCRIPT, *arguments)
        self.assertNotIn("Traceback", result.stderr)
        return result

    def reply(self, *arguments, ok=True):
        result = self.ctl(*arguments)
        self.assertEqual(result.returncode, 0 if ok else 1, result.stdout + result.stderr)
        answer = json.loads(result.stdout)
        self.assertEqual(answer["ok"], ok)
        return answer["msg"]


class TestLocalActions(CtlTest):
    def test_no_action(self):
        self.assertEqual(self.reply(ok=False), "no action")

    def test_unknown_action(self):
        self.configure()
        self.assertEqual(self.reply("dance", ok=False), "unknown action: dance")

    def test_pause_needs_no_config(self):
        paused = self.runtime / "paused"
        self.assertEqual(self.reply("pause"), "Paused")
        self.assertTrue(paused.exists())
        self.assertEqual(self.reply("pause", "toggle"), "Resumed")
        self.assertFalse(paused.exists())
        self.assertEqual(self.reply("pause", "off"), "Resumed")
        self.assertEqual(self.reply("pause", "on"), "Paused")
        self.assertEqual(self.reply("pause", "on"), "Paused")
        self.assertTrue(paused.exists())
        self.assertEqual(self.ssh_calls(), [])

    def test_log_without_a_log(self):
        result = self.ctl("log")
        self.assertEqual((result.returncode, result.stdout), (0, "(no log yet)\n"))

    def test_log_prints_the_last_lines(self):
        self.box.write(self.box.home / ".local" / "state" / "Linux-Router-Monitor" / "monitor.log",
                       "".join(f"line {n}\n" for n in range(1, 61)))
        self.assertEqual(self.ctl("log", "2").stdout, "line 59\nline 60\n")
        self.assertEqual(self.ctl("log").stdout.splitlines()[0], "line 11")

    def test_log_rejects_a_bad_count(self):
        self.assertEqual(self.reply("log", "0", ok=False), "invalid line count: 0")
        self.assertIn("invalid literal", self.reply("log", "many", ok=False))


class TestRouterActions(CtlTest):
    def setUp(self):
        super().setUp()
        self.configure()

    def test_missing_config_is_reported(self):
        self.config_path.unlink()
        self.assertIn("config.json", self.reply("reboot", ok=False))

    def test_invalid_config_is_reported(self):
        self.config_path.write_text("{")
        self.assertIn("Expecting property name", self.reply("reboot", ok=False))

    def test_unconfigured_host_is_reported(self):
        self.box.write(self.config_path, json.dumps({"host": ""}))
        self.assertEqual(self.reply("reboot", ok=False), "router host is not configured")
        self.assertEqual(self.ssh_calls(), [])

    def test_reboot_uses_the_shared_connection(self):
        self.assertEqual(self.reply("reboot"), "Reboot command sent")
        self.assertEqual(self.ssh_calls(), [[
            "-o", "BatchMode=yes", "-o", "ConnectTimeout=6", "-o", "ControlMaster=auto",
            "-o", f"ControlPath={self.runtime / 'cm.sock'}", "-o", "ControlPersist=60",
            "-o", "StrictHostKeyChecking=accept-new", "-i", str(self.box.home / ".ssh" / "id_ed25519"),
            "admin@router", "reboot"]])

    def test_configured_login_is_used(self):
        self.configure(user="root", ssh_key="~/keys/router", control_persist="5")
        self.reply("restart-wifi")
        call = self.ssh_calls()[0]
        self.assertIn("ControlPersist=5", call)
        self.assertIn(str(self.box.home / "keys" / "router"), call)
        self.assertEqual(call[-2:], ["root@router", "service restart_wireless"])

    def test_restart_wifi(self):
        self.assertEqual(self.reply("restart-wifi"), "WiFi restarting")

    def test_router_errors_are_reported(self):
        self.box.stub("ssh", stderr="ssh: connect to host router: No route to host\n", code=255)
        self.assertEqual(self.reply("reboot", ok=False), "ssh: connect to host router: No route to host")
        self.box.stub("ssh", stderr="", code=1)
        self.assertEqual(self.reply("restart-wifi", ok=False), "router command failed with exit code 1")

    def test_disconnect_deauthenticates_on_every_radio(self):
        self.assertEqual(self.reply("disconnect", "aa:bb:cc:00:00:01"), "Disconnect sent")
        self.assertEqual(self.remote_commands(), ["; ".join(
            f"wl -i {i} deauthenticate aa:bb:cc:00:00:01" for i in ("eth7", "eth8", "eth9", "eth10")) + "; true"])

    def test_actions_need_a_valid_mac(self):
        for action in ("disconnect", "block", "unblock", "block-status", "rename", "reserve"):
            self.assertEqual(self.reply(action, ok=False), "missing MAC address")
            self.assertEqual(self.reply(action, "aa:bb; reboot", "x", ok=False), "invalid MAC address: aa:bb; reboot")
        self.assertEqual(self.ssh_calls(), [])

    def test_block_and_unblock(self):
        check = "iptables -C FORWARD -m mac --mac-source aa:bb:cc:00:00:01 -j DROP 2>/dev/null"
        self.assertEqual(self.reply("block", "aa:bb:cc:00:00:01"), "Internet blocked")
        self.assertEqual(self.reply("unblock", "aa:bb:cc:00:00:01"), "Internet unblocked")
        self.assertEqual(self.remote_commands(), [
            f"{check} || iptables -I FORWARD -m mac --mac-source aa:bb:cc:00:00:01 -j DROP",
            f"while {check}; do iptables -D FORWARD -m mac --mac-source aa:bb:cc:00:00:01 -j DROP; done"])

    def test_block_status(self):
        self.assertEqual(self.reply("block-status", "aa:bb:cc:00:00:01"), "blocked")
        self.box.stub("ssh", code=1)
        self.assertEqual(self.reply("block-status", "aa:bb:cc:00:00:01"), "allowed")
        self.box.stub("ssh", stderr="Connection closed\n", code=255)
        self.assertEqual(self.reply("block-status", "aa:bb:cc:00:00:01", ok=False), "Connection closed")


class TestNvramActions(CtlTest):
    def setUp(self):
        super().setUp()
        self.configure()

    def test_rename_a_known_client(self):
        self.box.stub("ssh", stdout=CLIENTS + "\n", when="nvram get custom_clientlist")
        self.assertEqual(self.reply("rename", "aa:bb:cc:00:00:02", "Living room"), "Renamed to Living room")
        self.assertEqual(self.remote_commands()[1], "nvram set custom_clientlist='Laptop>AA:BB:CC:00:00:01>0>0>>>>><"
                         "Living room>AA:BB:CC:00:00:02>0>0>>>>>' && nvram commit")

    def test_rename_a_new_client(self):
        self.box.stub("ssh", stdout=CLIENTS, when="nvram get custom_clientlist")
        self.reply("rename", "aa:bb:cc:00:00:03", "Phone")
        self.assertEqual(self.remote_commands()[1], f"nvram set custom_clientlist='{CLIENTS}<Phone>AA:BB:CC:00:00:03>0>0>>>>>' "
                         "&& nvram commit")

    def test_rename_needs_a_valid_name(self):
        self.assertEqual(self.reply("rename", "aa:bb:cc:00:00:03", ok=False), "missing name")
        self.assertEqual(self.reply("rename", "aa:bb:cc:00:00:03", "a<b", ok=False), "invalid name: a<b")
        self.assertEqual(self.reply("rename", "aa:bb:cc:00:00:03", "  ", ok=False), "invalid name: ")

    def test_a_failed_read_never_overwrites_the_list(self):
        self.box.stub("ssh", stderr="timeout\n", code=255, when="nvram get custom_clientlist")
        self.assertEqual(self.reply("rename", "aa:bb:cc:00:00:03", "Phone", ok=False), "timeout")
        self.box.stub("ssh", stderr="timeout\n", code=255, when="nvram get dhcp_staticlist")
        self.assertEqual(self.reply("reserve", "aa:bb:cc:00:00:03", "192.168.1.20", ok=False), "timeout")
        self.assertEqual(self.remote_commands(), ["nvram get custom_clientlist", "nvram get dhcp_staticlist"])

    def test_reserve_a_known_client(self):
        self.box.stub("ssh", stdout=STATIC, when="nvram get dhcp_staticlist")
        self.assertEqual(self.reply("reserve", "aa:bb:cc:00:00:02", "192.168.1.50"), "Reserved 192.168.1.50")
        self.assertEqual(self.remote_commands()[1], "nvram set dhcp_staticlist='<AA:BB:CC:00:00:01>192.168.1.10>><AA:BB:CC:00:00:02>"
                         "192.168.1.50>>' && nvram set dhcp_static_x=1 && nvram commit && service restart_dnsmasq")

    def test_reserve_a_new_client(self):
        self.box.stub("ssh", stdout="", when="nvram get dhcp_staticlist")
        self.reply("reserve", "aa:bb:cc:00:00:03", "192.168.1.20")
        self.assertTrue(self.remote_commands()[1].startswith("nvram set dhcp_staticlist='<AA:BB:CC:00:00:03>192.168.1.20>>' "))

    def test_reserve_needs_a_valid_address(self):
        self.assertEqual(self.reply("reserve", "aa:bb:cc:00:00:03", ok=False), "missing IP address")
        self.assertIn("192.168.1.x", self.reply("reserve", "aa:bb:cc:00:00:03", "192.168.1.x", ok=False))
        self.assertEqual(self.ssh_calls(), [])


class TestProtection(CtlTest):
    def setUp(self):
        super().setUp()
        self.adguard = FakeAdGuard()
        self.addCleanup(self.adguard.close)
        self.configure(adguard={"url": self.adguard.url, "username": "admin", "password": "secret"})
        self.adguard.reply("/control/status", {"protection_enabled": True})

    def posted(self):
        return [request["body"] for request in self.adguard.requests if request["method"] == "POST"]

    def test_toggle_flips_the_current_state(self):
        self.assertEqual(self.reply("protection"), "Protection disabled")
        self.assertEqual(self.posted(), [{"enabled": False}])
        self.assertEqual({request["authorization"] for request in self.adguard.requests}, {basic("admin", "secret")})

    def test_on_and_off(self):
        self.assertEqual(self.reply("protection", "on"), "Protection enabled")
        self.assertEqual(self.reply("protection", "off"), "Protection disabled")
        self.assertEqual(self.posted(), [{"enabled": True}, {"enabled": False}])
        self.assertEqual(self.adguard.requests[1]["path"], "/control/protection")

    def test_status(self):
        self.assertEqual(json.loads(self.reply("status")), {"protection": True})
        self.adguard.reply("/control/status", "")
        self.assertEqual(json.loads(self.reply("status")), {"protection": None})

    def test_no_login_sends_no_authorization(self):
        self.configure(adguard={"url": self.adguard.url.rstrip("/")})
        self.reply("status")
        self.assertIsNone(self.adguard.requests[0]["authorization"])

    def test_http_errors_are_reported(self):
        self.adguard.reply("/control/protection", "denied", status=403)
        self.assertIn("403", self.reply("protection", "on", ok=False))

    def test_unreachable_adguard_is_reported(self):
        self.adguard.close()
        self.assertIn("Connection refused", self.reply("status", ok=False))

    def test_unconfigured_adguard_is_reported(self):
        self.configure(adguard={"url": ""})
        self.assertEqual(self.reply("status", ok=False), "AdGuard Home is not configured")
        self.configure()
        self.assertEqual(self.reply("protection", ok=False), "AdGuard Home is not configured")


class TestTimeouts(RouterTest):
    def test_a_router_timeout_is_reported(self):
        self.configure()
        module = load_script(SCRIPT, self.box.environment)
        output = io.StringIO()
        with mock.patch.object(module.subprocess, "run", side_effect=subprocess.TimeoutExpired("ssh", 25)), \
                mock.patch.object(sys, "argv", ["routermon-ctl", "reboot"]), contextlib.redirect_stdout(output), \
                self.assertRaises(SystemExit) as exit:
            module.main()
        self.assertEqual(exit.exception.code, 1)
        self.assertEqual(json.loads(output.getvalue()), {"ok": False, "msg": "Command 'ssh' timed out after 25 seconds"})


if __name__ == "__main__":
    unittest.main()
