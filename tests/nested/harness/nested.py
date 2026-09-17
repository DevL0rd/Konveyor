#!/usr/bin/env python3
import argparse
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import textwrap
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]


def build_dir():
    return Path(os.environ.get("KONVEYOR_BUILD_DIR", REPO / "build"))


class NestedSession:
    def __init__(self, width=1920, height=1080, config_kdl=None, extra_kwinrc="", global_shortcuts=False, xwayland=False, output_count=1):
        self.output_count = output_count
        self.width = width
        self.height = height
        self.root = Path(tempfile.mkdtemp(prefix="konveyor-test-"))
        self.socket = f"konveyor-test-{os.getpid()}"
        self.config_home = self.root / "config"
        self.data_home = self.root / "data"
        self.state_home = self.root / "state"
        self.config_home.mkdir()
        self.data_home.mkdir()
        self.state_home.mkdir()
        (self.config_home / "kwinrc").write_text(textwrap.dedent(f"""\
            [Plugins]
            konveyor_effectEnabled=true
            slideEnabled=false
            {extra_kwinrc}
            """))
        if config_kdl is not None:
            (self.config_home / "konveyor").mkdir()
            (self.config_home / "konveyor" / "config.kdl").write_text(config_kdl)
        self.global_shortcuts = global_shortcuts
        self.xwayland = xwayland
        self.proc = None
        self.log_path = self.root / "kwin.log"

    def env(self):
        env = {k: v for k, v in os.environ.items() if k not in ("WAYLAND_DISPLAY", "DISPLAY", "DBUS_SESSION_BUS_ADDRESS", "QT_QPA_PLATFORM", "KDE_FULL_SESSION", "XDG_CURRENT_DESKTOP", "SESSION_MANAGER")}
        env["XDG_CONFIG_HOME"] = str(self.config_home)
        env["XDG_DATA_HOME"] = str(self.data_home)
        env["XDG_STATE_HOME"] = str(self.state_home)
        env["QT_PLUGIN_PATH"] = f"{build_dir() / 'bin'}:{os.environ.get('QT_PLUGIN_PATH', '/usr/lib/qt6/plugins')}"
        env["KWIN_SCREENSHOT_NO_PERMISSION_CHECKS"] = "1"
        env["KWIN_WAYLAND_NO_PERMISSION_CHECKS"] = "1"
        env["QT_LOGGING_RULES"] = "kwin_*.debug=false"
        env["QT_FORCE_STDERR_LOGGING"] = "1"
        return env

    def start(self, session_script):
        script = self.root / "session.sh"
        script.write_text("#!/bin/sh\n" + session_script)
        script.chmod(0o755)
        log = open(self.log_path, "w")
        command = ["dbus-run-session", "--", "kwin_wayland", "--virtual", "--no-lockscreen"]
        if not self.global_shortcuts:
            command.append("--no-global-shortcuts")
        if self.xwayland:
            command.append("--xwayland")
        command += ["--socket", self.socket, "--width", str(self.width), "--height", str(self.height), "--output-count", str(self.output_count),
                    "--exit-with-session", str(script)]
        self.proc = subprocess.Popen(
            command,
            env=self.env(), stdout=log, stderr=subprocess.STDOUT, start_new_session=True,
        )
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
for name in A B C; do
    qml6 {client} -- $name 700 500 &
    sleep 2
done
sleep 3
python3 {runner} > "$KONVEYOR_REPORT" 2>&1
"""


def run_runner(runner, timeout, extra_config="", output_count=1):
    script = RUNNER_SCRIPT.format(client=REPO / "tests" / "nested" / "clients" / "client.qml", runner=runner)
    return run_script(script, timeout, extra_config, output_count=output_count)


def run_script(script, timeout, extra_config="", xwayland=False, output_count=1, extra_kwinrc=""):
    config = (REPO / "data" / "default-config.kdl").read_text() + extra_config
    session = NestedSession(config_kdl=config, extra_kwinrc=extra_kwinrc, xwayland=xwayland, output_count=output_count)
    report = session.root / "report.txt"
    session.start(f'export KONVEYOR_REPORT="{report}"\nexport KONVEYOR_KWIN_LOG="{session.log_path}"\n' + script)
    try:
        session.wait(timeout=timeout)
    except Exception as error:
        print(f"nested session did not finish: {error}")
    if not report.exists():
        print("the nested session produced no report")
        print("\n".join(session.log().splitlines()[-25:]))
        return 2
    text = report.read_text()
    print(text.strip())
    return 0 if "RESULT: PASS" in text else 1


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
