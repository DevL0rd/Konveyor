#!/usr/bin/env python3
import sys

from Xlib import X, Xatom, display
from Xlib.protocol import event


def announce(conn, title, request):
    conn.sync()
    print(f"konveyor-guard-request:{title}:{request}", flush=True)


def send_request(conn, root, window, net_wm_state, fullscreen):
    message = event.ClientMessage(window=window, client_type=net_wm_state, data=(32, [0, fullscreen, 0, 1, 0]))
    root.send_event(message, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
    announce(conn, window.get_wm_name(), "unfullscreen")


def send_minimize(conn, root, window, wm_change_state):
    message = event.ClientMessage(window=window, client_type=wm_change_state, data=(32, [3, 0, 0, 0, 0]))
    root.send_event(message, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
    announce(conn, window.get_wm_name(), "minimize")


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
    arm = conn.intern_atom("_KONVEYOR_TEST_ARM")
    active_atom = conn.intern_atom("_NET_ACTIVE_WINDOW")
    root.change_attributes(event_mask=X.PropertyChangeMask)
    window.map()
    conn.flush()
    mapped = False
    focused = False
    armed = False
    focus_phase = 0
    while True:
        received = conn.next_event()
        if received.type == X.MapNotify and not mapped:
            mapped = True
            message = event.ClientMessage(window=window, client_type=net_wm_state, data=(32, [1, fullscreen, 0, 1, 0]))
            root.send_event(message, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
            conn.flush()
        elif received.type == X.PropertyNotify and received.atom == arm:
            armed = True
        elif received.type == X.FocusIn or (received.type == X.PropertyNotify and received.atom == active_atom):
            focused = focused or received.type == X.FocusIn
            active = root.get_full_property(active_atom, Xatom.WINDOW)
            active_window = int(active.value[0]) if active is not None and len(active.value) else None
            if focus_phase == 1 and focused and active_window == window.id:
                focus_phase = 2
                if request == "unfullscreen":
                    send_request(conn, root, window, net_wm_state, fullscreen)
        elif received.type == X.FocusOut and focused and received.mode == X.NotifyNormal:
            focused = False
            if focus_phase == 0 and armed:
                focus_phase = 1
                if request == "minimize":
                    send_minimize(conn, root, window, wm_change_state)
                else:
                    send_request(conn, root, window, net_wm_state, fullscreen)


if __name__ == "__main__":
    main()
