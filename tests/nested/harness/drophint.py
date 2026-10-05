import os
import tempfile

from screenshot import capture_workspace, close

MAGENTA = (255, 0, 255)
WINDOW = (47, 48, 51)
DRAGGED_OPACITY = 0.75


def blend(over, under, alpha):
    return tuple(round(alpha * a + (1 - alpha) * b) for a, b in zip(over, under))


def screenshot():
    return capture_workspace(tempfile.mktemp(suffix=".png", dir=os.environ["KONVEYOR_TEST_ROOT"])).convert("RGB")


def pixel_close(point, color, tolerance=16):
    return close(screenshot().getpixel(point), color, tolerance)


def painted(image, *colors, step=4):
    points = [(x, y) for y in range(0, image.height, step) for x in range(0, image.width, step)
              if any(close(image.getpixel((x, y)), color) for color in colors)]
    if not points:
        return None
    xs, ys = [x for x, _ in points], [y for _, y in points]
    return min(xs), min(ys), max(xs) + step, max(ys) + step


def hint_area(color=MAGENTA):
    return painted(screenshot(), color, blend(WINDOW, color, DRAGGED_OPACITY))
