import time
from contextlib import contextmanager

from fakepointer import Held, move
from kwinsession import frame, konveyor_windows, wait_for

META = 125
LEFT_BUTTON = 0x110


def settled(title):
    previous = [None]

    def still():
        current, previous[0] = frame(title), frame(title)
        return current == previous[0] and current

    time.sleep(0.3)
    return wait_for(still, 10, 0.3)


def center(title):
    x, y, width, height = frame(title)
    return round(x + width / 2), round(y + height / 2)


def window_json(title):
    return next(window for window in konveyor_windows() if window["title"] == title)


def placement(title):
    return window_json(title)["layout"]["pos_in_scrolling_layout"]


@contextmanager
def dragging(title, goal, grab=None, release=True):
    start = grab or center(title)
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
