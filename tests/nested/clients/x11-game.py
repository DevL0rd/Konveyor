#!/usr/bin/env python3
import sys
import time

from Xlib import X, Xatom, Xutil, display
from Xlib.protocol import event

TITLE = sys.argv[1] if len(sys.argv) > 1 else "X11Game"
TAKES_FOCUS = "--take-focus" in sys.argv[2:]


def request_fullscreen(conn, root, window, state, fullscreen):
    message = event.ClientMessage(window=window, client_type=state, data=(32, [1, fullscreen, 0, 1, 0]))
    root.send_event(message, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
    conn.flush()


def use_take_focus(conn, window):
    window.set_wm_hints(flags=Xutil.InputHint, input=0)
    window.set_wm_protocols([conn.intern_atom("WM_TAKE_FOCUS")])


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
    protocols = conn.intern_atom("WM_PROTOCOLS")
    take_focus = conn.intern_atom("WM_TAKE_FOCUS")
    if TAKES_FOCUS:
        use_take_focus(conn, window)
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
        elif event_.type == X.ClientMessage and event_.client_type == protocols and event_.data[1][0] == take_focus:
            window.set_input_focus(X.RevertToParent, event_.data[1][1])
            conn.flush()


if __name__ == "__main__":
    main()
