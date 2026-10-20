#!/usr/bin/env python3
import os
import sys

import dbus
from PIL import Image

RAW_MODES = {4: "BGRA", 5: "BGRA", 6: "BGRA", 16: "RGBA", 17: "RGBA", 18: "RGBA"}


def capture_workspace(path):
    bus = dbus.SessionBus()
    iface = dbus.Interface(bus.get_object("org.kde.KWin.ScreenShot2", "/org/kde/KWin/ScreenShot2"), "org.kde.KWin.ScreenShot2")
    read_fd, write_fd = os.pipe()
    results = iface.CaptureWorkspace(dbus.Dictionary({"native-resolution": True}, signature="sv"), dbus.types.UnixFd(write_fd))
    os.close(write_fd)
    data = bytearray()
    with os.fdopen(read_fd, "rb") as pipe:
        while chunk := pipe.read(1 << 20):
            data.extend(chunk)
    width = int(results["width"])
    height = int(results["height"])
    stride = int(results["stride"])
    raw_mode = RAW_MODES[int(results["format"])]
    rows = [bytes(data[y * stride: y * stride + width * 4]) for y in range(height)]
    image = Image.frombytes("RGBA", (width, height), b"".join(rows), "raw", raw_mode)
    image.save(path)
    return image


if __name__ == "__main__":
    capture_workspace(sys.argv[1])


def close(pixel, color, tolerance=16):
    return all(abs(a - b) <= tolerance for a, b in zip(pixel[:3], color))
