import os
import re
import tempfile

from checks import default_config
from kwinsession import activate, active_title, frame, wait_for
from screenshot import capture_workspace, close

MAGENTA = (255, 0, 255)
CYAN = (0, 255, 255)
YELLOW = (255, 255, 0)
TOLERANCE = 48
TITLES = ("A", "B", "C")
NARROW = "window-rule {\n    default-column-width { proportion 0.3; }\n}\n"
RING_BLOCK = re.compile(r"    focus-ring \{.*?\n    \}\n", re.S)
BORDER_BLOCK = re.compile(r"    border \{.*?\n    \}\n", re.S)


def solid(name, color, width=8, extra=""):
    return f'border-style "{name}" {{\n    layer "gradient" {{ colors "{color}"; speed 0; width {width}; {extra} }}\n}}\n'


def probe_config(events, styles, ring="off", animations="", border=None, radius=None, gaps=None):
    config = default_config()
    if gaps is not None:
        config = config.replace("gaps 16", f"gaps {gaps}", 1)
    config = RING_BLOCK.sub(f"    focus-ring {{\n        {ring}\n    }}\n", config, count=1)
    if border is not None:
        config = BORDER_BLOCK.sub(f"    border {{\n        {border}\n    }}\n", config, count=1)
    if radius is not None:
        config = config.replace("geometry-corner-radius 12", f"geometry-corner-radius {radius}", 1)
    config = config.replace("\n    struts {", f"\n    event-effects {{\n{events}\n    }}\n\n    struts {{", 1)
    if animations:
        config = config.replace("animations {", f"animations {{\n    {animations}", 1)
    return config + NARROW + styles


def screenshot():
    return capture_workspace(tempfile.mktemp(suffix=".png", dir=os.environ["KONVEYOR_TEST_ROOT"]))


def edge_pixels(image, title, reach=7):
    geometry = frame(title)
    if geometry is None:
        return []
    x, y, width, height = (round(value) for value in geometry)
    pixels = []
    for distance in range(1, reach):
        for row in (y + height // 4, y + height // 2, y + 3 * height // 4):
            for column in (x - distance, x + width + distance - 1):
                if 0 <= column < image.width and 0 <= row < image.height:
                    pixels.append(image.getpixel((column, row))[:3])
        for column in (x + width // 4, x + width // 2, x + 3 * width // 4):
            for row in (y - distance, y + height + distance - 1):
                if 0 <= column < image.width and 0 <= row < image.height:
                    pixels.append(image.getpixel((column, row))[:3])
    return pixels


def painted(image, title, color):
    return any(close(pixel, color, TOLERANCE) for pixel in edge_pixels(image, title))


def painted_titles(image, titles, color):
    return {title for title in titles if painted(image, title, color)}


def settle_to(title):
    activate(title)
    wait_for(lambda: active_title() == title)


def expect_painted(checks, color, expected, message, timeout=20):
    seen = set()

    def matches():
        nonlocal seen
        seen = painted_titles(screenshot(), TITLES, color)
        return seen == set(expected)

    checks.expect(wait_for(matches, timeout, 0.3), f"{message} (painted {sorted(seen)}, expected {sorted(expected)})")


def left_pixel(image, title, distance):
    x, y, _, height = (round(value) for value in frame(title))
    return image.getpixel((x - distance, y + height // 2))[:3]
