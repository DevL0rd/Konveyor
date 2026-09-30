#!/usr/bin/env python3
import sys

from Xlib import X, Xatom, display


def atom(conn, name):
    return conn.intern_atom(name)


def window_type(conn, name):
    prefix = "_KDE_NET_WM_WINDOW_TYPE_" if name in ("ON_SCREEN_DISPLAY", "CRITICAL_NOTIFICATION", "APPLET_POPUP") else "_NET_WM_WINDOW_TYPE_"
    return atom(conn, prefix + name)


def create(conn, spec, parents):
    title, kind, *flags = spec.split(":")
    screen = conn.screen()
    window = screen.root.create_window(40, 40, 400, 300, 0, screen.root_depth, X.InputOutput, X.CopyFromParent,
                                       background_pixel=screen.black_pixel, event_mask=X.StructureNotifyMask,
                                       override_redirect="override" in flags)
    window.set_wm_name(title)
    window.set_wm_class("x11types", "X11Types")
    window.change_property(atom(conn, "_NET_WM_WINDOW_TYPE"), Xatom.ATOM, 32, [window_type(conn, kind)])
    states = [atom(conn, "_NET_WM_STATE_SKIP_TASKBAR")] if "skip-taskbar" in flags else []
    states += [atom(conn, "_NET_WM_STATE_MODAL")] if "modal" in flags else []
    if states:
        window.change_property(atom(conn, "_NET_WM_STATE"), Xatom.ATOM, 32, states)
    if "transient" in flags:
        window.change_property(atom(conn, "WM_TRANSIENT_FOR"), Xatom.WINDOW, 32, [parents[0].id])
    window.map()
    conn.flush()
    return window


def main():
    conn = display.Display()
    windows = []
    for spec in sys.argv[1:]:
        windows.append(create(conn, spec, windows))
    while True:
        conn.next_event()


if __name__ == "__main__":
    main()
