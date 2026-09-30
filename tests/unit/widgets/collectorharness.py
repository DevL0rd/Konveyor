import builtins
import contextlib
import glob
import importlib.machinery
import importlib.util
import io
import json
import os
import sys
import tempfile
import unittest
import warnings
from pathlib import Path
from unittest import mock

sys.dont_write_bytecode = True
REPO = Path(__file__).resolve().parents[3]
WIDGETS = REPO / "widgets"
FAKED = ("/proc", "/sys")
RECORDING_STUB = """#!/usr/bin/env python3
import json, os, sys
with open(os.environ["STUB_LOG"], "a") as log:
    log.write(json.dumps([os.path.basename(sys.argv[0])] + sys.argv[1:]) + "\\n")
"""


class FakeRoot:
    def __init__(self, root):
        self.root = str(root)

    def faked(self, path):
        return isinstance(path, str) and any(path == prefix or path.startswith(prefix + "/") for prefix in FAKED)

    def map(self, path):
        return self.root + path if self.faked(path) else path

    def unmap(self, path):
        return path[len(self.root):] if path.startswith(self.root + "/") else path

    def write(self, path, text):
        target = Path(self.root + path)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)
        return target

    def patches(self):
        real_open, real_os_open, real_listdir, real_stat, real_glob = builtins.open, os.open, os.listdir, os.stat, glob.glob
        real_exists = os.path.exists
        return [
            mock.patch("builtins.open", lambda path, *a, **k: real_open(self.map(path), *a, **k)),
            mock.patch("os.open", lambda path, *a, **k: real_os_open(self.map(path), *a, **k)),
            mock.patch("os.listdir", lambda path=".": real_listdir(self.map(path))),
            mock.patch("os.stat", lambda path, *a, **k: real_stat(self.map(path), *a, **k)),
            mock.patch("os.path.exists", lambda path: real_exists(self.map(path))),
            mock.patch("glob.glob", lambda pattern, *a, **k: [self.unmap(p) for p in real_glob(self.map(pattern), *a, **k)]),
        ]


def load_script(path, name):
    loader = importlib.machinery.SourceFileLoader(name, str(path))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    return module


def write_stub(directory, name, text):
    path = Path(directory) / name
    path.write_text(text)
    path.chmod(0o755)
    return path


class CollectorTest(unittest.TestCase):
    def setUp(self):
        warnings.simplefilter("ignore", ResourceWarning)
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.home = Path(temporary.name)
        self.stubs = self.home / "bin"
        self.stubs.mkdir()
        self.runtime = self.home / "run"
        self.runtime.mkdir()
        self.config = self.home / "config"
        self.log = self.home / "calls"
        self.environment = {**os.environ, "HOME": str(self.home), "XDG_RUNTIME_DIR": str(self.runtime),
                            "XDG_CONFIG_HOME": str(self.config), "XDG_DATA_HOME": str(self.home / "data"),
                            "PATH": f"{self.stubs}:{os.environ['PATH']}", "STUB_LOG": str(self.log),
                            "DBUS_SESSION_BUS_ADDRESS": "unix:path=/nonexistent/konveyor-test-bus"}
        self.fake = FakeRoot(self.home / "root")

    def load(self, path, name, pynvml=None):
        self.start(mock.patch.dict(os.environ, self.environment, clear=True))
        self.start(mock.patch.dict(sys.modules, {"pynvml": pynvml}))
        for patch in self.fake.patches():
            self.start(patch)
        return load_script(path, name)

    def start(self, patch):
        patch.start()
        self.addCleanup(patch.stop)

    def calls(self):
        return [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []


def run_main(module, name, *arguments):
    output = io.StringIO()
    errors = io.StringIO()
    with mock.patch.object(module.sys, "argv", [name, *arguments]), mock.patch.object(module.time, "sleep"), \
            contextlib.redirect_stdout(output), contextlib.redirect_stderr(errors):
        code = module.main()
    return code, output.getvalue(), errors.getvalue()
