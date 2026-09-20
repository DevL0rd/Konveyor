#!/usr/bin/env python3
import sys
import time
from pathlib import Path

from Xlib import X, Xatom, display
from Xlib.protocol import event

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import activate, window_minimized, window_state


def minimize_x11(title):
    conn = display.Display()
    root = conn.screen().root
    client_list = conn.intern_atom("_NET_CLIENT_LIST")
    clients = root.get_full_property(client_list, Xatom.WINDOW)
    windows = [conn.create_resource_object("window", window) for window in clients.value] if clients else []
    target = next((window for window in windows if window.get_wm_name() == title), None)
    if target is None:
        return False
    wm_change_state = conn.intern_atom("WM_CHANGE_STATE")
    message = event.ClientMessage(window=target, client_type=wm_change_state, data=(32, [3, 0, 0, 0, 0]))
    root.send_event(message, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
    conn.flush()
    return True


def x11_wm_state(title):
    conn = display.Display()
    root = conn.screen().root
    clients = root.get_full_property(conn.intern_atom("_NET_CLIENT_LIST"), Xatom.WINDOW)
    windows = [conn.create_resource_object("window", window) for window in clients.value] if clients else []
    target = next((window for window in windows if window.get_wm_name() == title), None)
    if target is None:
        return None
    state = target.get_full_property(conn.intern_atom("WM_STATE"), conn.intern_atom("WM_STATE"))
    return int(state.value[0]) if state is not None and len(state.value) else None


def check_requests(title, problems):
    activate(title)
    time.sleep(0.5)
    activate("A")
    time.sleep(1)
    state = window_state(title)
    minimized = window_minimized(title)
    print(f"{title} after inactive request: {state}, minimized {minimized}")
    if not state or not state.startswith("true|"):
        problems.append(f"{title} left fullscreen after its inactive request ({state})")
    if minimized:
        problems.append(f"{title} minimized after its inactive request")
    if title == "X11GuardMinimize" and x11_wm_state(title) != 3:
        problems.append(f"{title} did not receive the fake iconic acknowledgement")
    activate(title)
    time.sleep(1)
    refocused = window_state(title)
    minimized = window_minimized(title)
    print(f"{title} after active request: {refocused}, minimized {minimized}")
    if title.endswith("GuardMinimize"):
        if minimized or not refocused or not refocused.startswith("true|"):
            problems.append(f"{title} accepted minimize while fullscreen ({refocused}, minimized {minimized})")
        if title.startswith("X11") and x11_wm_state(title) != 1:
            problems.append(f"{title} did not return to normal state when refocused")
    elif refocused and refocused.startswith("true|"):
        problems.append(f"{title} did not accept its windowed request while active ({refocused})")
    elif title.startswith("X11Guard"):
        if not minimize_x11(title):
            problems.append(f"{title} could not be found for its windowed minimize check")
        time.sleep(1)
        if not window_minimized(title):
            problems.append(f"{title} did not accept minimize after becoming windowed")
    elif not minimized:
        problems.append(f"{title} did not accept minimize after becoming windowed")


def main():
    problems = []
    time.sleep(2)
    check_requests("GuardUnfullscreen", problems)
    check_requests("GuardMinimize", problems)
    check_requests("X11GuardMinimize", problems)
    check_requests("X11GuardUnfullscreen", problems)
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
