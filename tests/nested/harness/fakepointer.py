import os
import subprocess
import time
from functools import cache
from pathlib import Path

CLIENTS = Path(__file__).resolve().parent.parent / "clients"


@cache
def binary():
    build = Path(os.environ["KONVEYOR_TEST_ROOT"]) / "fakepointer"
    build.mkdir(exist_ok=True)
    protocol = CLIENTS / "fake-input.xml"
    subprocess.run(["wayland-scanner", "client-header", protocol, build / "fake-input-client-protocol.h"], check=True)
    subprocess.run(["wayland-scanner", "private-code", protocol, build / "fake-input-protocol.c"], check=True)
    output = build / "fakepointer"
    subprocess.run(["cc", f"-I{build}", CLIENTS / "fakepointer.c", build / "fake-input-protocol.c", "-lwayland-client", "-lm", "-o", output], check=True)
    return output


def fake(*arguments, settle=0.0):
    subprocess.run([binary(), *(str(argument) for argument in arguments)], check=True)
    time.sleep(settle)


def move(x, y, settle=True):
    fake("move", x, y, settle=0.3 if settle else 0)


def click(x, y, settle=True):
    move(x, y, settle)
    fake("click", x, y, settle=0.5 if settle else 0)


def touch(fingers, x, y, dx=0, dy=0, radius=0, end_radius=None, hold_ms=0, steps=20, settle=True):
    end = radius if end_radius is None else end_radius
    fake("touch", fingers, x, y, dx, dy, radius, end, hold_ms, steps, settle=1.0 if settle else 0)


def press(x, y):
    fake("press", x, y)


def release(x, y):
    fake("release", x, y)


def keys(*events):
    fake("keys", *(event if isinstance(event, str) else f"{event[0]}:{event[1]}" for event in events))


def chord(modifiers, *events):
    keys(*[(code, 1) for code in modifiers], *events, *[(code, 0) for code in reversed(modifiers)])


def tap(modifiers, key):
    chord(modifiers, (key, 1), (key, 0))


class Held:
    def __enter__(self):
        self.process = subprocess.Popen([binary(), "stdin"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
        return self

    def send(self, *events):
        for event in events:
            self.process.stdin.write((event if isinstance(event, str) else f"{event[0]}:{event[1]}") + "\n")
            self.process.stdin.flush()
            if self.process.stdout.readline().strip() != "done":
                raise RuntimeError(f"the fake input client did not take {event}")

    def glide(self, start, end, steps=12):
        self.send(*(f"move:{start[0] + (end[0] - start[0]) * step / steps}:{start[1] + (end[1] - start[1]) * step / steps}" for step in range(steps + 1)))

    def __exit__(self, *exc):
        self.process.stdin.close()
        self.process.wait(timeout=30)
