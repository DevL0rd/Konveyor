import importlib.machinery
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
PORTAL_BIN = REPO / "widgets" / "portals" / "bin"
RECORDING_STUB = """#!%s
import json, os, sys
name = os.path.basename(sys.argv[0])
with open(os.environ["STUB_LOG"], "a") as log:
    log.write(json.dumps([name] + sys.argv[1:]) + "\\n")
candidates = [os.path.join(os.environ["STUB_REPLIES"], name + "-" + argument) for argument in sys.argv[1:]]
replies = next((c for c in candidates if any(os.path.isfile(c + s) for s in (".out", ".err", ".code"))),
               os.path.join(os.environ["STUB_REPLIES"], name))
if os.path.isfile(replies + ".out"):
    sys.stdout.write(open(replies + ".out").read())
if os.path.isfile(replies + ".err"):
    sys.stderr.write(open(replies + ".err").read())
sys.exit(int(open(replies + ".code").read()) if os.path.isfile(replies + ".code") else 0)
""" % sys.executable


class Sandbox:
    def __init__(self, stubs=(), tools=()):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.home = self.root / "home"
        self.config = self.home / ".config"
        self.data = self.home / ".local" / "share"
        self.system_data = self.root / "system"
        self.runtime = self.root / "runtime"
        self.stubs = self.root / "bin"
        self.replies = self.root / "replies"
        self.log = self.root / "calls"
        for directory in (self.home, self.system_data, self.runtime, self.stubs, self.replies):
            directory.mkdir(parents=True, exist_ok=True)
        for tool in stubs:
            self.stub(tool)
        for tool in tools:
            (self.stubs / tool).symlink_to(shutil.which(tool))
        self.environment = {
            "PATH": str(self.stubs),
            "HOME": str(self.home),
            "XDG_CONFIG_HOME": str(self.config),
            "XDG_DATA_HOME": str(self.data),
            "XDG_DATA_DIRS": str(self.system_data),
            "XDG_CACHE_HOME": str(self.home / ".cache"),
            "XDG_STATE_HOME": str(self.home / ".local" / "state"),
            "XDG_RUNTIME_DIR": str(self.runtime),
            "STUB_LOG": str(self.log),
            "STUB_REPLIES": str(self.replies),
        }

    def cleanup(self):
        self.temporary.cleanup()

    def stub(self, tool, stdout=None, stderr=None, code=None, when=None):
        (self.stubs / tool).write_text(RECORDING_STUB)
        (self.stubs / tool).chmod(0o755)
        name = tool + "-" + when if when else tool
        for suffix, value in ((".out", stdout), (".err", stderr), (".code", code)):
            if value is not None:
                (self.replies / (name + suffix)).write_text(str(value))

    def calls(self):
        return [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []

    def write(self, path, text):
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        return path

    def desktop(self, directory, name, **fields):
        body = "".join(f"{key}={value}\n" for key, value in fields.items())
        return self.write(Path(directory) / "applications" / name, "[Desktop Entry]\n" + body)

    def run(self, program, *arguments):
        return subprocess.run([sys.executable, str(program), *arguments], env=self.environment, capture_output=True, text=True)

    def run_json(self, program, *arguments):
        result = self.run(program, *arguments)
        if result.returncode != 0:
            raise AssertionError(result.stderr)
        return json.loads(result.stdout)


def load_script(path, environment):
    saved = dict(os.environ)
    os.environ.update(environment)
    try:
        loader = importlib.machinery.SourceFileLoader(Path(path).name.replace("-", "_"), str(path))
        spec = importlib.util.spec_from_loader(loader.name, loader)
        module = importlib.util.module_from_spec(spec)
        loader.exec_module(module)
    finally:
        os.environ.clear()
        os.environ.update(saved)
    return module
