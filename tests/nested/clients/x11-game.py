#!/usr/bin/env python3
import sys
import time

from Xlib import X, Xatom, display
from Xlib.protocol import event

TITLE = sys.argv[1] if len(sys.argv) > 1 else "X11Game"


def request_fullscreen(conn, root, window, state, fullscreen):
    message = event.ClientMessage(window=window, client_type=state, data=(32, [1, fullscreen, 0, 1, 0]))
    root.send_event(message, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
    conn.flush()


def main():
    conn = display.Display()
    screen = conn.screen()
    root = screen.root
    window = root.create_window(0, 0, 800, 600, 0, screen.root_depth, X.InputOutput, X.CopyFromParent,
                                background_pixel=screen.white_pixel,
                                event_mask=X.PropertyChangeMask | X.StructureNotifyMask)
    window.set_wm_name(TITLE)
    window.set_wm_class("x11game", "X11Game")
    state = conn.intern_atom("_NET_WM_STATE")
    fullscreen = conn.intern_atom("_NET_WM_STATE_FULLSCREEN")
    window.map()
    conn.flush()
    time.sleep(1.5)
    request_fullscreen(conn, root, window, state, fullscreen)
    while True:
        event_ = conn.next_event()
        if event_.type == X.PropertyNotify and event_.atom == state:
            current = window.get_full_property(state, Xatom.ATOM)
            if current is None or fullscreen not in current.value:
                request_fullscreen(conn, root, window, state, fullscreen)


if __name__ == "__main__":
    main()
