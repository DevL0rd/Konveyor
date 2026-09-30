import hashlib
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
sys.path.insert(0, str(HERE / "fakes"))

import kconfig
import state

STUBS = ("sudo", "runuser", "systemctl", "systemd-run", "busctl", "gdbus", "kwriteconfig6", "kreadconfig6", "kpackagetool6",
         "kbuildsycoca6", "cmake", "konveyor-kontrol-panel", "konveyor", "toolbox", "distrobox", "podman", "rpm", "dpkg", "id",
         "getent", "date", "sleep", "journalctl", "kscreen-doctor", "jq", "ssh", "curl")
MANAGERS = ("pacman", "dnf", "zypper", "apt-get")
TOOLS = ("bash", "sh", "cat", "cp", "mv", "rm", "rmdir", "mkdir", "ln", "chmod", "find", "sed", "grep", "sort", "comm", "basename",
         "dirname", "tee", "install", "touch", "head", "cut", "tr", "awk", "join", "xargs", "sha256sum", "readlink", "env", "mktemp",
         "ls", "python3", "git", "wc", "uname", "tar", "gzip")
OS_RELEASE = {
    "arch": "ID=arch\n",
    "fedora-atomic": "ID=fedora\nVERSION_ID=42\nOSTREE_VERSION=42.20250101.0\n",
    "steamos": "ID=steamos\nID_LIKE=arch\nVERSION_ID=3.9\nBUILD_ID=20250101.1\n",
}
NOBODY = 65534
KWIN = "6.7.0"
SOURCE_FUNCTIONS = """
set -euo pipefail
source "$1"
SYSTEM_UPDATE_ROOT=false
eval "$2"
"""


class Harness:
    def __init__(self, managers=("pacman",), system="arch", checkout=True):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.home = self.root / "home"
        self.system = self.root / "system"
        self.stubs = self.root / "bin"
        self.state = self.root / "stub"
        self.source = self.root / "source"
        self.prefix = self.root / "prefix"
        self.atomic = system != "arch"
        for directory in (self.state, self.root / "runtime", self.home / ".config", self.home / ".local" / "share",
                          self.home / ".local" / "state"):
            directory.mkdir(parents=True)
        self.make_path(managers)
        self.make_system(system)
        self.environment = {
            "PATH": f"{self.stubs}:{self.root / 'tools'}",
            "HOME": str(self.home),
            "USER": "tester",
            "LANG": "C.UTF-8",
            "XDG_CONFIG_HOME": str(self.home / ".config"),
            "XDG_DATA_HOME": str(self.home / ".local" / "share"),
            "XDG_STATE_HOME": str(self.home / ".local" / "state"),
            "XDG_RUNTIME_DIR": str(self.root / "runtime"),
            "XDG_DATA_DIRS": f"{self.system}/usr/local/share:{self.system}/usr/share",
            "DBUS_SESSION_BUS_ADDRESS": f"unix:path={self.root / 'no-bus'}",
            "KONVEYOR_SYSTEM_ROOT": str(self.system),
            "STUB_DIR": str(self.state),
            "STUB_BIN": str(self.stubs),
            "STUB_OWNER_UID": "1000",
            "STUB_OWNER_HOME": str(self.home),
            "GIT_CONFIG_NOSYSTEM": "1",
            "GIT_CONFIG_GLOBAL": str(self.root / "gitconfig"),
            "GIT_ALLOW_PROTOCOL": "file",
        }
        if not self.atomic:
            self.environment["KONVEYOR_PREFIX"] = str(self.prefix)
        (self.root / "gitconfig").write_text("[user]\n\tname = t\n\temail = t@t\n[safe]\n\tdirectory = *\n[init]\n\tdefaultBranch = main\n[protocol \"file\"]\n\tallow = always\n")
        self.make_source(checkout)
        self.write(self.home / ".config" / "kwinrc", "[Plugins]\nblurEnabled=true\n")
        self.set_shortcut("plasmashell", "activate application launcher", ["Meta", "Alt+F1"], "Plasma", "Activate Application Launcher")
        self.set_shortcut("kwin", "Grid View", ["Meta+G"], "KWin", "Toggle Grid View")

    def close(self):
        self.temporary.cleanup()

    def make_path(self, managers):
        self.stubs.mkdir()
        fakes = shutil.copytree(HERE / "fakes", self.root / "fakes", ignore=shutil.ignore_patterns("__pycache__"))
        for name in dict.fromkeys(STUBS + tuple(managers)):
            (self.stubs / name).symlink_to(fakes / "stub.py")
        tools = self.root / "tools"
        tools.mkdir()
        for name in TOOLS:
            (tools / name).symlink_to(shutil.which(name))

    def make_system(self, system):
        self.write(self.system / "etc" / "os-release", OS_RELEASE[system])
        if system == "fedora-atomic":
            self.write(self.system / "run" / "ostree-booted", "")
        library = self.system / "usr" / "lib"
        if system == "steamos":
            self.write(self.system / "etc" / "pacman.conf", "[options]\n[jupiter-3.9]\n[holo-3.9]\n")
        for name, version in (("libkwin", KWIN), ("libQt6Core", "6.9.1")):
            self.write(library / f"{name}.so.{version}", "")
            (library / f"{name}.so.6").symlink_to(f"{name}.so.{version}")
        self.set_headers(KWIN)
        self.write(library / "libalpm.so.15", "NetworkAccess\n")

    def set_headers(self, version):
        self.write(self.system / "usr" / "lib" / "cmake" / "KWin" / "KWinConfigVersion.cmake", f'set(PACKAGE_VERSION "{version}")\n')

    def start_session(self):
        path = self.system / "run" / "user" / "1000" / "bus"
        path.parent.mkdir(parents=True, exist_ok=True)
        listener = socket.socket(socket.AF_UNIX)
        listener.bind(str(path))
        listener.close()

    def make_source(self, checkout):
        self.source.mkdir()
        for name in ("install.sh", "uninstall.sh"):
            shutil.copy2(REPO / name, self.source / name)
        shutil.copytree(REPO / "extras", self.source / "extras")
        shutil.copytree(REPO / "widgets", self.source / "widgets", ignore=shutil.ignore_patterns(".git", "__pycache__"))
        if not checkout:
            return
        self.git("init", "-q")
        self.git("add", "-A")
        self.git("commit", "-qm", "Initial")
        self.git("clone", "-q", "--bare", str(self.source), str(self.root / "origin.git"), cwd=self.root)
        self.git("remote", "add", "origin", str(self.root / "origin.git"))
        self.git("fetch", "-q", "origin")
        self.git("branch", "-q", "--set-upstream-to=origin/main")

    def git(self, *arguments, cwd=None):
        return subprocess.run(["git", *arguments], cwd=cwd or self.source, env=self.environment, check=True,
                              capture_output=True, text=True).stdout.strip()

    def push_upstream_commit(self, name="upstream.txt", content=None):
        other = self.root / "other"
        if not other.exists():
            self.git("clone", "-q", str(self.root / "origin.git"), str(other), cwd=self.root)
        (other / name).write_text(name if content is None else content)
        self.git("add", name, cwd=other)
        self.git("commit", "-qm", f"Add {name}", cwd=other)
        self.git("push", "-q", "origin", "main", cwd=other)

    def seed_prefix(self, stamp="1"):
        build = self.root / "seed-build"
        for arguments in (["-S", str(self.source), "-B", str(build), f"-DCMAKE_INSTALL_PREFIX={self.prefix}",
                           "-DKDE_INSTALL_USE_QT_SYS_PATHS=OFF"], ["--install", str(build)]):
            subprocess.run([str(self.stubs / "cmake"), *arguments], env=self.environment, check=True)
        state_dir = self.prefix / "share" / "konveyor"
        shutil.copy2(build / "install_manifest.txt", state_dir / "install_manifest.txt")
        plugins = self.prefix / "lib" / "plugins" / "kwin" / "effects" / "plugins"
        for plugin in ("konveyor_effect", "process_monitor_telemetry"):
            (plugins / f"{plugin}.so").rename(plugins / f"{plugin}_{stamp}.so")
            (state_dir / ("plugin-id" if plugin == "konveyor_effect" else "telemetry-plugin-id")).write_text(f"{plugin}_{stamp}\n")
        shutil.rmtree(build)
        self.entries()
        return plugins

    def write(self, path, text):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)

    def run(self, *command, root=False, env=None):
        wrapper = []
        if root and os.geteuid() != 0:
            wrapper = [shutil.which("unshare"), "--user", "--map-root-user", "--"]
        if not root and os.geteuid() == 0:
            for path in [self.root, *self.root.rglob("*")]:
                os.lchown(path, NOBODY, NOBODY)
            wrapper = [shutil.which("setpriv"), f"--reuid={NOBODY}", f"--regid={NOBODY}", "--clear-groups", "--"]
        return subprocess.run([*wrapper, str(self.root / "tools" / "bash"), *command], env={**self.environment, **(env or {})},
                              capture_output=True, text=True, stdin=subprocess.DEVNULL)

    def install(self, *arguments, **options):
        return self.run(str(self.source / "install.sh"), *arguments, **options)

    def uninstall(self, *arguments, **options):
        return self.run(str(self.source / "uninstall.sh"), *arguments, **options)

    def functions(self, snippet, script="install.sh", **options):
        return self.run("-c", SOURCE_FUNCTIONS, "functions", str(self.source / script), snippet, **options)

    def entries(self):
        log = self.state / "calls.jsonl"
        entries = [json.loads(line) for line in log.read_text().splitlines()] if log.exists() else []
        log.unlink(missing_ok=True)
        return entries

    def calls(self, name=None):
        return [entry["argv"] for entry in self.entries() if name is None or entry["argv"][0] == name]

    def stub_state(self, name, default=None):
        return json.loads((self.state / f"{name}.json").read_text()) if (self.state / f"{name}.json").exists() else default

    def set_stub_state(self, name, value):
        (self.state / f"{name}.json").write_text(json.dumps(value))

    def set_shortcut(self, component, action, keys, component_name=None, action_name=None):
        store = self.stub_state("kglobalaccel", {})
        names = [component, action, component_name or component, action_name or action]
        store[state.action_name(component, action)] = {"keys": [state.key_code(key) for key in keys], "names": names}
        self.set_stub_state("kglobalaccel", store)

    def shortcut(self, component, action):
        entry = self.stub_state("kglobalaccel", {}).get(state.action_name(component, action))
        return None if entry is None else [state.key_text(code) for code in entry["keys"]]

    def effect_takes(self, *keys):
        store = self.stub_state("kglobalaccel", {})
        path = self.home / ".local" / "state" / "konveyorstaterc"
        for key in keys:
            code = state.key_code(key)
            for holder in state.holders(store, code):
                component, action = holder.split("\t")
                group = ["ReleasedShortcuts", f"{component}/{action}"]
                if not kconfig.entry(path, group, "Component"):
                    for field, value in zip(("Component", "Action", "ComponentFriendlyName", "ActionFriendlyName"), store[holder]["names"]):
                        kconfig.set_entry(path, group, field, value)
                    kconfig.set_entry(path, group, "Keys", ",".join(state.key_text(code) for code in store[holder]["keys"]))
                store[holder]["keys"].remove(code)
            store[state.action_name("konveyor", key)] = {"keys": [code], "names": ["konveyor", key, "Konveyor", key]}
        self.set_stub_state("kglobalaccel", store)

    def config(self, name):
        return kconfig.read(self.home / ".config" / name)

    def snapshot(self):
        files = {}
        for base in (self.home, self.system, self.prefix):
            for path in sorted(base.rglob("*")) if base.exists() else []:
                relative = str(path.relative_to(self.root))
                if "/.git/" in relative:
                    continue
                if path.is_symlink():
                    files[relative] = "-> " + os.readlink(path)
                elif path.is_dir():
                    files[relative] = "dir"
                elif path.name.endswith("rc"):
                    files[relative] = kconfig.read(path)
                else:
                    files[relative] = hashlib.sha256(path.read_bytes()).hexdigest()
        for name in ("kglobalaccel", "systemd"):
            files[f"stub:{name}"] = self.stub_state(name)
        return files


class HarnessTest(unittest.TestCase):
    managers = ("pacman",)
    system = "arch"
    checkout = True

    def setUp(self):
        self.harness = Harness(self.managers, self.system, self.checkout)
        self.addCleanup(self.harness.close)

    def assertSucceeded(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result
