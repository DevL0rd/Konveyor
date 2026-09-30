#!/usr/bin/env python3
import argparse
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
HARNESS = Path(__file__).resolve().parent

BUS_CONFIG = """<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>session</type>
  <keep_umask/>
  <listen>unix:tmpdir=/tmp</listen>
  <auth>EXTERNAL</auth>
  <policy context="default">
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
    <allow own="*"/>
  </policy>
</busconfig>
"""

NOTIFICATIONS_PRELUDE = """
python3 {server} "{log}" &
for _ in $(seq 200); do [ -e "{log}.ready" ] && break; sleep 0.05; done
"""


def build_dir():
    return Path(os.environ.get("KONVEYOR_BUILD_DIR", REPO / "build"))


class NestedSession:
    def __init__(self, width=1920, height=1080, config_kdl=None, extra_kwinrc="", global_shortcuts=False, xwayland=False, output_count=1,
                 input_method=None, files=None, notifications=False, hidden_data=()):
        self.output_count = output_count
        self.width = width
        self.height = height
        self.root = Path(tempfile.mkdtemp(prefix="konveyor-test-"))
        self.socket = f"konveyor-test-{os.getpid()}-{self.root.name}"
        self.config_home = self.root / "config"
        self.data_home = self.root / "data"
        self.state_home = self.root / "state"
        self.home = self.root / "home"
        for directory in (self.config_home, self.data_home, self.state_home, self.home):
            directory.mkdir()
        (self.config_home / "kwinrc").write_text(textwrap.dedent(f"""\
            [Plugins]
            konveyor_effectEnabled=true
            slideEnabled=false
            {extra_kwinrc}
            """))
        if config_kdl is not None:
            (self.config_home / "konveyor").mkdir()
            (self.config_home / "konveyor" / "config.kdl").write_text(config_kdl)
        for relative, text in (files or {}).items():
            path = self.root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
        (self.root / "bus.conf").write_text(BUS_CONFIG)
        self.data_dirs = self.hide_data(hidden_data) if hidden_data else None
        self.global_shortcuts = global_shortcuts
        self.xwayland = xwayland
        self.input_method = input_method
        self.notifications = notifications
        self.notifications_log = self.root / "notifications.jsonl"
        self.proc = None
        self.log_path = self.root / "kwin.log"

    def hide_data(self, hidden):
        overlays = []
        for index, directory in enumerate(os.environ.get("XDG_DATA_DIRS", "/usr/local/share:/usr/share").split(":")):
            overlay = self.root / "data-dirs" / str(index)
            overlay.mkdir(parents=True)
            for entry in Path(directory).glob("*"):
                if entry.name not in hidden:
                    (overlay / entry.name).symlink_to(entry)
            overlays.append(str(overlay))
        return ":".join(overlays)

    def env(self):
        env = {k: v for k, v in os.environ.items() if k not in ("WAYLAND_DISPLAY", "DISPLAY", "DBUS_SESSION_BUS_ADDRESS", "QT_QPA_PLATFORM", "KDE_FULL_SESSION", "XDG_CURRENT_DESKTOP", "SESSION_MANAGER")}
        env["HOME"] = str(self.home)
        env["XDG_CONFIG_HOME"] = str(self.config_home)
        env["XDG_DATA_HOME"] = str(self.data_home)
        env["XDG_STATE_HOME"] = str(self.state_home)
        if self.data_dirs:
            env["XDG_DATA_DIRS"] = self.data_dirs
        env["KONVEYOR_TEST_ROOT"] = str(self.root)
        env["KONVEYOR_NOTIFICATIONS_LOG"] = str(self.notifications_log)
        env["QT_PLUGIN_PATH"] = f"{build_dir() / 'bin'}:{os.environ.get('QT_PLUGIN_PATH', '/usr/lib/qt6/plugins')}"
        env["KWIN_SCREENSHOT_NO_PERMISSION_CHECKS"] = "1"
        env["KWIN_WAYLAND_NO_PERMISSION_CHECKS"] = "1"
        env["QT_LOGGING_RULES"] = "kwin_*.debug=false"
        env["QT_FORCE_STDERR_LOGGING"] = "1"
        env["KSCREEN_BACKEND"] = "KWayland"
        env["KSCREEN_BACKEND_INPROCESS"] = "1"
        return env

    def kwin_arguments(self, script):
        arguments = ["--virtual", "--no-lockscreen"]
        if not self.global_shortcuts:
            arguments.append("--no-global-shortcuts")
        if self.xwayland:
            arguments.append("--xwayland")
        if self.input_method:
            arguments += ["--inputmethod", self.input_method]
        return arguments + ["--socket", self.socket, "--width", str(self.width), "--height", str(self.height),
                            "--output-count", str(self.output_count), "--exit-with-session", str(script)]

    def start(self, session_script):
        script = self.root / "session.sh"
        script.write_text("#!/bin/sh\n" + session_script)
        script.chmod(0o755)
        prelude = NOTIFICATIONS_PRELUDE.format(server=HARNESS / "notifications.py", log=self.notifications_log) if self.notifications else ""
        log = open(self.log_path, "w")
        command = ["dbus-run-session", f"--config-file={self.root / 'bus.conf'}", "--", "sh", "-c", prelude + 'exec kwin_wayland "$@"', "kwin",
                   *self.kwin_arguments(script)]
        self.proc = subprocess.Popen(command, env=self.env(), stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        return self.proc

    def wait(self, timeout):
        try:
            return self.proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(self.proc.pid, signal.SIGTERM)
            self.proc.wait(timeout=10)
            raise

    def log(self):
        return self.log_path.read_text(errors="replace")

    def cleanup(self):
        shutil.rmtree(self.root, ignore_errors=True)


RUNNER_SCRIPT = """
export QT_QPA_PLATFORM=wayland
python3 {clients} {client} A B C > "$KONVEYOR_REPORT" 2>&1 && python3 {runner} {arguments} > "$KONVEYOR_REPORT" 2>&1
"""


def run_runner(runner, timeout, extra_config="", client="client.qml", arguments="", **session):
    script = RUNNER_SCRIPT.format(clients=HARNESS / "clients.py", client=client, runner=runner, arguments=arguments)
    return run_script(script, timeout, extra_config, **session)


def run_script(script, timeout, extra_config="", config_kdl=None, write_config=True, **session):
    config = (REPO / "data" / "default-config.kdl").read_text() + extra_config if config_kdl is None else config_kdl
    session = NestedSession(config_kdl=config if write_config else None, **session)
    report = session.root / "report.txt"
    session.start(f'export KONVEYOR_REPORT="{report}"\nexport KONVEYOR_KWIN_LOG="{session.log_path}"\n' + script)
    try:
        session.wait(timeout=timeout)
    except Exception as error:
        print(f"nested session did not finish: {error}")
    if not report.exists():
        print("the nested session produced no report")
        print("\n".join(session.log().splitlines()[-25:]))
        session.cleanup()
        return 2
    text = report.read_text()
    print(text.strip())
    passed = "RESULT: PASS" in text
    if not passed:
        print("\n".join(session.log().splitlines()[-40:]))
    session.cleanup()
    return 0 if passed else 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("script", help="shell snippet run inside the nested session")
    parser.add_argument("--timeout", type=int, default=60)
    parser.add_argument("--keep", action="store_true")
    args = parser.parse_args()
    session = NestedSession()
    session.start(Path(args.script).read_text())
    try:
        code = session.wait(args.timeout)
    finally:
        print(session.log())
        if not args.keep:
            session.cleanup()
        else:
            print("kept", session.root)
    sys.exit(code)


if __name__ == "__main__":
    main()
