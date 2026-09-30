#!/usr/bin/env python3
import base64
import json
import os
import stat
import unittest
from pathlib import Path
from unittest import mock

from portal_harness import load_script
from router_harness import ROUTER_BIN, RouterTest


SCRIPT = ROUTER_BIN / "routermon-config"
COLLECTOR = ROUTER_BIN.parent / "router" / "collect.sh"
CONNECTION = ("router.lan", "root", "~/.ssh/router", "/jffs/collect.sh")
KEYGEN = """entry = {"tool": "ssh-keygen", "argv": sys.argv[1:]}
open(os.environ["STUB_LOG"] + ".tools", "a").write(json.dumps(entry) + "\\n")
target = sys.argv[sys.argv.index("-f") + 1]
if "-y" in sys.argv:
    print("ssh-ed25519 AAAAderived router")
else:
    open(target, "w").write("PRIVATE\\n")
    open(target + ".pub", "w").write("ssh-ed25519 AAAAfresh router\\n")
"""
SETSID = """askpass = os.environ.get("SSH_ASKPASS", "")
entry = {"tool": "setsid", "argv": sys.argv[1:], "askpass": askpass, "require": os.environ.get("SSH_ASKPASS_REQUIRE"),
         "answer": subprocess.run([askpass], capture_output=True, text=True).stdout}
open(os.environ["STUB_LOG"] + ".tools", "a").write(json.dumps(entry) + "\\n")
sys.stderr.write(os.environ.get("SETSID_ERROR", ""))
sys.exit(int(os.environ.get("SETSID_CODE", "0")))
"""
SSH = """entry = {"tool": "ssh", "argv": sys.argv[1:], "stdin": sys.stdin.buffer.read().decode()}
open(os.environ["STUB_LOG"] + ".tools", "a").write(json.dumps(entry) + "\\n")
sys.stderr.write(os.environ.get("SSH_ERROR", ""))
sys.exit(int(os.environ.get("SSH_CODE", "0")))
"""


class TestRoutermonConfig(RouterTest):
    stubs = ()

    def setUp(self):
        super().setUp()
        self.script_stub("ssh-keygen", KEYGEN)
        self.script_stub("setsid", SETSID)
        self.script_stub("ssh", SSH)
        self.key = self.box.home / ".ssh" / "router"

    def config(self, *arguments, code=0):
        result = self.invoke(SCRIPT, *arguments)
        self.assertEqual(result.returncode, code, result.stderr)
        self.assertNotIn("Traceback", result.stderr)
        return result

    def saved(self):
        return json.loads(self.config_path.read_text())

    def test_get_without_a_config_shows_the_defaults(self):
        for arguments in ((), ("get",)):
            self.assertEqual(json.loads(self.config(*arguments).stdout),
                             {"host": "", "user": "admin", "ssh_key": "~/.ssh/id_ed25519", "remote_script": "/jffs/lrm-collect.sh"})

    def test_get_shows_only_the_connection(self):
        self.box.write(self.config_path, json.dumps({"host": "10.0.0.1", "user": "me", "ssh_key": "k", "remote_script": "r",
                                                     "adguard": {"password": "secret"}}))
        self.assertEqual(self.config("get").stdout, '{"host":"10.0.0.1","user":"me","ssh_key":"k","remote_script":"r"}\n')

    def test_save_keeps_other_settings_and_fills_defaults(self):
        self.box.write(self.config_path, json.dumps({"adguard": {"url": "http://agh"}, "poll_interval": 2}))
        self.assertEqual(self.config("save", " router.lan ", *CONNECTION[1:]).stdout, "Connection settings saved\n")
        self.assertEqual(self.saved(), {"adguard": {"url": "http://agh"}, "poll_interval": 2, "host": "router.lan", "user": "root",
                                        "ssh_key": "~/.ssh/router", "remote_script": "/jffs/collect.sh", "cache_ttl": 0.45,
                                        "ping_target": "8.8.8.8", "control_persist": "60"})
        self.assertEqual(stat.S_IMODE(self.config_path.stat().st_mode), 0o600)
        self.assertEqual([path.name for path in self.config_path.parent.iterdir()], ["config.json"])

    def test_the_config_is_never_readable_by_others(self):
        module = load_script(SCRIPT, self.box.environment)
        modes = []

        def watched(real):
            return lambda path, *arguments: (modes.append(stat.S_IMODE(path.stat().st_mode)), real(path, *arguments))[1]

        previous = os.umask(0o022)
        try:
            with mock.patch.object(Path, "replace", watched(Path.replace)), mock.patch.object(Path, "chmod", watched(Path.chmod)):
                module.save_config({"host": "router", "adguard": {"password": "secret"}})
        finally:
            os.umask(previous)
        self.assertEqual(set(modes), {0o600})
        self.assertEqual(self.saved()["adguard"], {"password": "secret"})

    def test_save_needs_every_field(self):
        self.assertEqual(self.config("save", "router", "root", "key", code=1).stderr,
                         "host, user, SSH key and remote script are required\n")
        self.assertEqual(self.config("save", "router", " ", "key", "script", code=1).stderr, "connection fields cannot be empty\n")
        self.assertFalse(self.config_path.exists())

    def test_unknown_mode(self):
        self.assertEqual(self.config("wipe", code=1).stderr, "usage: routermon-config [get|save|test|install|authorize|connect]\n")

    def test_test_connection(self):
        self.assertEqual(self.config("test", *CONNECTION).stdout, "SSH connection succeeded\n")
        self.assertEqual(self.tool_calls("ssh")[0]["argv"], ["-o", "BatchMode=yes", "-o", "ConnectTimeout=8", "-o",
                                                             "StrictHostKeyChecking=accept-new", "-i", str(self.key),
                                                             "root@router.lan", "true"])
        self.assertEqual(self.saved()["host"], "router.lan")

    def test_failed_connection_reports_the_last_line(self):
        self.box.environment.update(SSH_CODE="255", SSH_ERROR="Warning: added host\nroot@router.lan: Permission denied\n")
        self.assertEqual(self.config("test", *CONNECTION, code=1).stderr, "root@router.lan: Permission denied\n")
        self.box.environment["SSH_ERROR"] = ""
        self.assertEqual(self.config("test", *CONNECTION, code=1).stderr, "SSH authentication failed\n")

    def test_install_copies_the_collector(self):
        self.assertEqual(self.config("install", "router.lan", "root", "~/.ssh/router", "/jffs/my scripts/c.sh").stdout,
                         "Router collector installed\n")
        call = self.tool_calls("ssh")[0]
        self.assertEqual(call["argv"][-1], "cat > '/jffs/my scripts/c.sh' && chmod +x '/jffs/my scripts/c.sh'")
        self.assertEqual(call["stdin"], COLLECTOR.read_text())

    def test_failed_install_is_reported(self):
        self.box.environment.update(SSH_CODE="1", SSH_ERROR="sh: can't create /jffs/collect.sh: Read-only file system\n")
        self.assertEqual(self.config("install", *CONNECTION, code=1).stderr,
                         "sh: can't create /jffs/collect.sh: Read-only file system\n")
        self.box.environment["SSH_ERROR"] = ""
        self.assertEqual(self.config("install", *CONNECTION, code=1).stderr, "collector installation failed\n")

    def password(self, text):
        return base64.b64encode(text.encode()).decode()

    def test_authorize_creates_a_key_and_answers_the_password_prompt(self):
        self.assertEqual(self.config("authorize", *CONNECTION, self.password("hunter 2")).stdout, "SSH key authorized\n")
        keygen = self.tool_calls("ssh-keygen")
        self.assertEqual([call["argv"] for call in keygen], [["-q", "-t", "ed25519", "-N", "", "-f", str(self.key)]])
        setsid = self.tool_calls("setsid")[0]
        self.assertEqual(setsid["argv"], ["-w", "ssh-copy-id", "-f", "-i", str(self.key) + ".pub", "root@router.lan"])
        self.assertEqual((setsid["require"], setsid["answer"]), ("force", "hunter 2\n"))
        self.assertTrue(setsid["askpass"].startswith(str(self.box.root / "tmp") + "/"))
        self.assertEqual(list((self.box.root / "tmp").iterdir()), [])

    def test_authorize_derives_a_missing_public_key(self):
        self.box.write(self.key, "PRIVATE\n")
        self.config("authorize", *CONNECTION, self.password("pw"))
        self.assertEqual([call["argv"] for call in self.tool_calls("ssh-keygen")], [["-y", "-f", str(self.key)]])
        public = self.key.with_name("router.pub")
        self.assertEqual(public.read_text(), "ssh-ed25519 AAAAderived router\n")
        self.assertEqual(stat.S_IMODE(public.stat().st_mode), 0o644)

    def test_authorize_keeps_an_existing_key_pair(self):
        self.box.write(self.key, "PRIVATE\n")
        self.box.write(self.key.with_name("router.pub"), "ssh-ed25519 AAAAold\n")
        self.config("authorize", *CONNECTION, self.password("pw"))
        self.assertEqual(self.tool_calls("ssh-keygen"), [])

    def test_authorize_rejects_bad_passwords(self):
        self.assertEqual(self.config("authorize", *CONNECTION, "not base64!", code=1).stderr, "invalid password payload\n")
        self.assertEqual(self.config("authorize", *CONNECTION, code=1).stderr, "router password is empty\n")
        self.assertEqual(self.config("authorize", *CONNECTION, self.password(""), code=1).stderr, "router password is empty\n")
        self.assertEqual(self.tool_calls(), [])

    def test_failed_authorization_is_reported(self):
        self.box.environment.update(SETSID_CODE="1", SETSID_ERROR="Permission denied, please try again.\n")
        self.assertEqual(self.config("authorize", *CONNECTION, self.password("pw"), code=1).stderr,
                         "Permission denied, please try again.\n")
        self.box.environment["SETSID_ERROR"] = ""
        self.assertEqual(self.config("connect", *CONNECTION, self.password("pw"), code=1).stderr, "SSH key authorization failed\n")
        self.assertEqual(self.tool_calls("ssh"), [])

    def test_connect_authorizes_then_installs(self):
        result = self.config("connect", *CONNECTION, self.password("pw"))
        self.assertEqual(result.stdout, "SSH key authorized\nRouter collector installed\nRouter connected\n")
        self.assertEqual([call["tool"] for call in self.tool_calls()], ["ssh-keygen", "setsid", "ssh"])

    def test_a_missing_tool_is_reported(self):
        (self.box.stubs / "ssh").unlink()
        self.assertIn("No such file or directory: 'ssh'", self.config("test", *CONNECTION, code=1).stderr)


if __name__ == "__main__":
    unittest.main()
