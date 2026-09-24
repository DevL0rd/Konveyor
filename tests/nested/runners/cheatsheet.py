#!/usr/bin/env python3
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import run_script
from nested import REPO, build_dir

SCRIPT = build_dir() / "src" / "cheatsheet" / "konveyor-cheatsheet"


def script_environment(root):
    tools = root / "bin"
    tools.mkdir()
    (tools / "python3").symlink_to(sys.executable)
    runtime = root / "runtime"
    runtime.mkdir(mode=0o700)
    display = Path(os.environ["XDG_RUNTIME_DIR"]) / os.environ["WAYLAND_DISPLAY"]
    environment = dict(os.environ, HOME=str(root), PATH=str(tools), XDG_RUNTIME_DIR=str(runtime), WAYLAND_DISPLAY=str(display),
                       KONVEYOR_CHEATSHEET_QML=str(REPO / "src" / "cheatsheet" / "Cheatsheet.qml"),
                       KONVEYOR_CHEATSHEET_VIEWER=str(build_dir() / "bin" / "konveyor-cheatsheet-viewer"))
    return environment, runtime / "konveyor-cheatsheet.pid"


def window_pids():
    return [int(pid) for pid in run_script('for (const w of workspace.windowList()) { if (!w.deleted) { print("MARK|" + w.pid); } }')]


def wait_until(condition, seconds):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        value = condition()
        if value:
            return value
        time.sleep(0.5)
    return condition()


def read_pid(pid_file):
    return int(pid_file.read_text()) if pid_file.exists() else None


def main():
    problems = []
    with tempfile.TemporaryDirectory() as temporary:
        environment, pid_file = script_environment(Path(temporary))
        first = subprocess.Popen([str(SCRIPT)], env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        viewer = wait_until(lambda: read_pid(pid_file) if first.poll() is None else -1, 20)
        if viewer in (None, -1):
            output = first.communicate(timeout=10)[0] if first.poll() is not None else ""
            problems.append(f"the cheatsheet did not start (exit {first.poll()}): {output.strip()}")
        elif not wait_until(lambda: viewer in window_pids(), 20):
            problems.append(f"no window from the cheatsheet viewer (pid {viewer}) appeared")
        else:
            print(f"cheatsheet window open (viewer pid {viewer})")
            second = subprocess.run([str(SCRIPT)], env=environment, capture_output=True, text=True, timeout=30)
            print(f"second run exited {second.returncode}")
            if second.returncode != 0:
                problems.append(f"running it again failed: {second.stdout}{second.stderr}")
            try:
                first.wait(timeout=15)
            except subprocess.TimeoutExpired:
                problems.append("the first cheatsheet did not close")
            if not wait_until(lambda: viewer not in window_pids(), 10):
                problems.append("the cheatsheet window is still open")
            if pid_file.exists():
                problems.append("the pid file was left behind")
            print("cheatsheet closed" if not problems else "cheatsheet did not close cleanly")
        if first.poll() is None:
            first.kill()
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
