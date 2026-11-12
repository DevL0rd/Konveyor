#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from borderprobe import probe_config, screenshot, settle_to
from checks import Checks, load_config
from kwinsession import frame, wait_for
from screenshot import close

MAGENTA = (255, 0, 255)
RING = 8
TOLERANCE = 48


def config(radius, clip):
    ring = f'width {RING}\n        active-color "#ff00ff"\n        inactive-color "#ff00ff"'
    text = probe_config("", "", ring=ring, radius=radius)
    return text.replace("clip-to-geometry true", f"clip-to-geometry {'true' if clip else 'false'}", 1)


def ring_corner():
    image = screenshot()
    x, y, _, _ = (round(value) for value in frame("B"))
    return [image.getpixel((x - RING + step, y - RING + step)) for step in (3, 4, 5)]


def square(pixels):
    return all(close(pixel, MAGENTA, TOLERANCE) for pixel in pixels)


def expect_corner(checks, radius, clip, wanted_square):
    checks.expect(load_config(config(radius, clip)), f"radius {radius} clip {clip}: config loaded")
    pixels = []

    def settled():
        nonlocal pixels
        pixels = ring_corner()
        return square(pixels) == wanted_square

    shape = "square" if wanted_square else "rounded"
    checks.expect(wait_for(settled, 10, 0.3), f"radius {radius} clip {clip}: the ring's outer corner is {shape} ({pixels})")


def rounded_then_square(checks):
    settle_to("B")
    expect_corner(checks, 12, True, False)
    expect_corner(checks, 0, True, True)
    expect_corner(checks, 0, False, True)
    expect_corner(checks, 12, False, False)
    expect_corner(checks, 0, False, True)


if __name__ == "__main__":
    Checks().run(rounded_then_square)
