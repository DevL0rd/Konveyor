import subprocess
import tempfile
import time
from functools import cache
from pathlib import Path

CLIENTS = Path(__file__).resolve().parent.parent / "clients"


@cache
def binary():
    build = Path(tempfile.mkdtemp(prefix="konveyor-fakepointer-"))
    protocol = CLIENTS / "fake-input.xml"
    subprocess.run(["wayland-scanner", "client-header", protocol, build / "fake-input-client-protocol.h"], check=True)
    subprocess.run(["wayland-scanner", "private-code", protocol, build / "fake-input-protocol.c"], check=True)
    output = build / "fakepointer"
    subprocess.run(["cc", f"-I{build}", CLIENTS / "fakepointer.c", build / "fake-input-protocol.c", "-lwayland-client", "-lm", "-o", output], check=True)
    return output


def move(x, y):
    subprocess.run([binary(), "move", str(x), str(y)], check=True)
    time.sleep(0.3)


def click(x, y):
    move(x, y)
    subprocess.run([binary(), "click", str(x), str(y)], check=True)
    time.sleep(0.5)


def touch(fingers, x, y, dx=0, dy=0, radius=0, end_radius=None, hold_ms=0, steps=20):
    end = radius if end_radius is None else end_radius
    subprocess.run([binary(), "touch", *(str(value) for value in (fingers, x, y, dx, dy, radius, end, hold_ms, steps))], check=True)
    time.sleep(1.0)
