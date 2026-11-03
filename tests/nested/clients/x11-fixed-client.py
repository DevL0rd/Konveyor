#!/usr/bin/env python3
import select
import sys

from Xlib import X, Xutil, display

TITLE, APP_ID = sys.argv[1], sys.argv[2]


def declare(window, width, height):
    window.set_wm_normal_hints(flags=Xutil.PMinSize | Xutil.PMaxSize, min_width=width, min_height=height, max_width=width, max_height=height)


def main():
    conn = display.Display()
    screen = conn.screen()
    window = screen.root.create_window(0, 0, 500, 400, 0, screen.root_depth, X.InputOutput, X.CopyFromParent,
                                       background_pixel=screen.white_pixel, event_mask=X.StructureNotifyMask)
    window.set_wm_name(TITLE)
    window.set_wm_class(APP_ID, APP_ID)
    declare(window, 500, 400)
    window.map()
    conn.flush()
    size = (500, 400)
    commands = ""
    while True:
        ready, _, _ = select.select([conn.fileno(), sys.stdin], [], [])
        if sys.stdin in ready:
            line = sys.stdin.readline()
            if not line:
                return
            width, height = (int(value) for value in line.split()[1].split("x"))
            declare(window, width, height)
            window.configure(width=width, height=height)
            conn.flush()
            size = (width, height)
            print(f"CHOSE {width}x{height}", flush=True)
        while conn.pending_events():
            event = conn.next_event()
            if event.type == X.ConfigureNotify and (event.width, event.height) != size:
                size = (event.width, event.height)
                declare(window, *size)
                conn.flush()
                print(f"FOLLOWED {size[0]}x{size[1]}", flush=True)


if __name__ == "__main__":
    main()
