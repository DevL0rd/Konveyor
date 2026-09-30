from contextlib import contextmanager

from fakepointer import Held, move
from kwinsession import frame, konveyor_windows, run_script, wait_for
from nested import REPO

META = 125
LEFT_BUTTON = 0x110


def drag_config(hint=None):
    config = (REPO / "data" / "default-config.kdl").read_text()
    moving = config.replace('titlebar-drag "scroll-view"', 'titlebar-drag "move-window"')
    if hint is not None:
        moving = moving.replace("    gaps 16\n", "    gaps 16\n    insert-hint {\n" + hint + "    }\n", 1)
    if moving.count('titlebar-drag "move-window"') != 1 or (hint is not None and moving.count("insert-hint") != 1):
        raise RuntimeError("the default config no longer has the titlebar-drag and gaps lines the drag tests rewrite")
    return moving


def settled(title):
    previous = [None]

    def still():
        current = frame(title)
        stable = current == previous[0]
        previous[0] = current
        return stable and current

    return wait_for(still, 10, 0.3)


def center(title):
    x, y, width, height = frame(title)
    return round(x + width / 2), round(y + height / 2)


def grab_point(title, output_left=0, output_width=1920):
    x, y, width, height = frame(title)
    left, right = max(x, output_left) + 40, min(x + width, output_left + output_width) - 40
    return round((left + right) / 2), round(y + min(height / 2, 540))


def quick_tiling():
    return run_script('print("MARK|" + options.electricBorderTiling + "|" + options.electricBorderMaximize);')[0]


def window_json(title):
    return next(window for window in konveyor_windows() if window["title"] == title)


def placement(title):
    return window_json(title)["layout"]["pos_in_scrolling_layout"]


@contextmanager
def dragging(title, goal, grab=None, release=True):
    start = grab or grab_point(title)
    lifted = (start[0], start[1] - 300 if start[1] > 540 else start[1] + 300)
    with Held() as pointer:
        pointer.send((META, 1), f"move:{start[0]}:{start[1]}", f"button:{LEFT_BUTTON}:1")
        pointer.glide(start, lifted, 8)
        pointer.glide(lifted, goal, 16)
        yield pointer
        if release:
            pointer.send(f"button:{LEFT_BUTTON}:0")
        pointer.send((META, 0))
    move(960, 1070)
