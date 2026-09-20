#!/usr/bin/env python3
import sys
import time

from Xlib import X, Xatom, display
from Xlib.protocol import event


def send_request(conn, root, window, net_wm_state, fullscreen):
    message = event.ClientMessage(window=window, client_type=net_wm_state, data=(32, [0, fullscreen, 0, 1, 0]))
    root.send_event(message, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
    conn.flush()


def send_minimize(conn, root, window, wm_change_state):
    message = event.ClientMessage(window=window, client_type=wm_change_state, data=(32, [3, 0, 0, 0, 0]))
    root.send_event(message, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
    conn.flush()


def main():
    title = sys.argv[1]
    request = sys.argv[2]
    conn = display.Display()
    screen = conn.screen()
    root = screen.root
    window = root.create_window(0, 0, 800, 600, 0, screen.root_depth, X.InputOutput, X.CopyFromParent,
                                background_pixel=screen.white_pixel,
                                event_mask=X.FocusChangeMask | X.PropertyChangeMask | X.StructureNotifyMask)
    window.set_wm_name(title)
    window.set_wm_class("x11guard", "X11Guard")
    net_wm_state = conn.intern_atom("_NET_WM_STATE")
    fullscreen = conn.intern_atom("_NET_WM_STATE_FULLSCREEN")
    wm_change_state = conn.intern_atom("WM_CHANGE_STATE")
    window.map()
    conn.flush()
    time.sleep(1.5)
    message = event.ClientMessage(window=window, client_type=net_wm_state, data=(32, [1, fullscreen, 0, 1, 0]))
    root.send_event(message, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
    conn.flush()
    focused = False
    activations = 0
    focus_phase = 0
    while True:
        received = conn.next_event()
        if received.type == X.FocusIn:
            if not focused:
                focused = True
                activations += 1
            active = root.get_full_property(conn.intern_atom("_NET_ACTIVE_WINDOW"), Xatom.WINDOW)
            active_window = int(active.value[0]) if active is not None and len(active.value) else None
            if focus_phase == 1 and active_window == window.id:
                focus_phase = 2
                if request == "unfullscreen":
                    send_request(conn, root, window, net_wm_state, fullscreen)
        elif received.type == X.FocusOut and focused:
            focused = False
            if focus_phase == 0 and activations >= 2:
                focus_phase = 1
                if request == "minimize":
                    send_minimize(conn, root, window, wm_change_state)
                else:
                    send_request(conn, root, window, net_wm_state, fullscreen)


if __name__ == "__main__":
    main()
