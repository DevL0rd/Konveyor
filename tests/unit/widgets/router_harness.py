import base64
import http.server
import json
import subprocess
import sys
import threading
import unittest

from portal_harness import REPO, Sandbox


ROUTER_BIN = REPO / "widgets" / "router-monitor" / "bin"
APP = "Linux-Router-Monitor"


class FakeAdGuard:
    def __init__(self):
        self.requests = []
        self.replies = {}
        fake = self

        class Handler(http.server.BaseHTTPRequestHandler):
            def log_message(self, *arguments):
                pass

            def handle_one(self, method):
                length = int(self.headers.get("Content-Length") or 0)
                body = self.rfile.read(length).decode() if length else None
                fake.requests.append({"method": method, "path": self.path, "body": json.loads(body) if body else None,
                                      "authorization": self.headers.get("Authorization")})
                status, text = fake.replies.get(self.path, (200, ""))
                data = text.encode()
                self.send_response(status)
                self.send_header("Content-Length", str(len(data)))
                self.end_headers()
                self.wfile.write(data)

            def do_GET(self):
                self.handle_one("GET")

            def do_POST(self):
                self.handle_one("POST")

        self.server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        self.url = f"http://127.0.0.1:{self.server.server_address[1]}/"
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()

    def reply(self, path, value, status=200):
        self.replies[path] = (status, value if isinstance(value, str) else json.dumps(value))

    def close(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()


def basic(user, password):
    return "Basic " + base64.b64encode(f"{user}:{password}".encode()).decode()


class RouterTest(unittest.TestCase):
    stubs = ("ssh",)
    linked = ()

    def setUp(self):
        self.box = Sandbox(stubs=self.stubs, tools=self.linked)
        self.addCleanup(self.box.cleanup)
        self.box.environment["DBUS_SESSION_BUS_ADDRESS"] = "unix:path=/nonexistent/konveyor-test-bus"
        self.box.environment["TMPDIR"] = str(self.box.root / "tmp")
        (self.box.root / "tmp").mkdir()
        self.config_path = self.box.config / APP / "config.json"
        self.runtime = self.box.runtime / APP

    def configure(self, **fields):
        self.box.write(self.config_path, json.dumps({"host": "router", **fields}))

    def ssh_calls(self):
        return [call[1:] for call in self.box.calls() if call[0] == "ssh"]

    def remote_commands(self):
        return [call[-1] for call in self.ssh_calls()]

    def invoke(self, program, *arguments):
        return subprocess.run([sys.executable, str(program), *arguments], env=self.box.environment, capture_output=True,
                              text=True, stdin=subprocess.DEVNULL)

    def script_stub(self, name, body):
        path = self.box.stubs / name
        path.write_text(f"#!{sys.executable}\nimport json, os, subprocess, sys\n" + body)
        path.chmod(0o755)
        return path

    def tool_calls(self, tool=None):
        path = self.box.log.with_name(self.box.log.name + ".tools")
        calls = [json.loads(line) for line in path.read_text().splitlines()] if path.exists() else []
        return [call for call in calls if tool is None or call["tool"] == tool]
