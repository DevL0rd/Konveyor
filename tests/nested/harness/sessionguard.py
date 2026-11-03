import os
import subprocess
from pathlib import Path

PREFIX = "konveyor-test-"


def in_harness_session():
    socket = os.environ.get("WAYLAND_DISPLAY", "")
    root = os.environ.get("KONVEYOR_TEST_ROOT", "")
    return bool(socket.startswith(PREFIX) and root and Path(root).is_dir() and socket.endswith("-" + Path(root).name))


def require_harness_session(what):
    if not in_harness_session():
        raise SystemExit(f"refusing to start {what}: this is not a nested harness session (WAYLAND_DISPLAY={os.environ.get('WAYLAND_DISPLAY', '')!r}, "
                         f"expected a {PREFIX}* socket). Run it through its test_*.py wrapper so it opens only inside the nested KWin, never on the real desktop")


def popen(command, **options):
    require_harness_session(command[0])
    return subprocess.Popen(command, **options)
