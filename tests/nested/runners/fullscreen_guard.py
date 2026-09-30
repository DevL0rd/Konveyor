#!/usr/bin/env python3
import os
import re
import subprocess
import sys
from pathlib import Path

from Xlib import X, Xatom, display
from Xlib.protocol import event

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import CLIENTS, activate, kwin_titles, reload_konveyor, run_script, wait_for, window_minimized, window_state
from nested import build_dir

LOGS = Path(os.environ["KONVEYOR_TEST_ROOT"])
REQUEST = re.compile(r"-> xdg_toplevel[#@][0-9]+[.](unset_fullscreen|set_minimized)[(]")


def launch(title, command, environment=None):
    with open(LOGS / f"{title}.log", "w") as log:
        subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT, env=dict(os.environ, **(environment or {})))
    if not wait_for(lambda: title in kwin_titles(), 60):
        raise RuntimeError(f"{title} never appeared")
    activate(title)


def requests_sent(title):
    lines = (LOGS / f"{title}.log").read_text(errors="replace").splitlines()
    if title.startswith("X11"):
        return sum(line.startswith(f"konveyor-guard-request:{title}:") for line in lines)
    return sum(bool(REQUEST.search(line)) for line in lines)


def after_request(title, trigger):
    before = requests_sent(title)
    trigger()
    if not wait_for(lambda: requests_sent(title) > before, 60):
        raise RuntimeError(f"{title} never sent its request")
    run_script("")


def x11_window(conn, title):
    root = conn.screen().root
    clients = root.get_full_property(conn.intern_atom("_NET_CLIENT_LIST"), Xatom.WINDOW)
    windows = [conn.create_resource_object("window", window) for window in clients.value] if clients else []
    return next((window for window in windows if window.get_wm_name() == title), None)


def minimize_x11(title):
    conn = display.Display()
    target = x11_window(conn, title)
    if target is None:
        return False
    message = event.ClientMessage(window=target, client_type=conn.intern_atom("WM_CHANGE_STATE"), data=(32, [3, 0, 0, 0, 0]))
    conn.screen().root.send_event(message, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
    conn.flush()
    return True


def arm_x11(title):
    conn = display.Display()
    target = x11_window(conn, title)
    target.change_property(conn.intern_atom("_KONVEYOR_TEST_ARM"), Xatom.CARDINAL, 32, [1])
    conn.sync()


def x11_wm_state(title):
    conn = display.Display()
    target = x11_window(conn, title)
    if target is None:
        return None
    state = target.get_full_property(conn.intern_atom("WM_STATE"), conn.intern_atom("WM_STATE"))
    return int(state.value[0]) if state is not None and len(state.value) else None


def is_fullscreen(title):
    return (window_state(title) or "").startswith("true|")


def check_requests(title, problems):
    activate(title)
    if title.startswith("X11"):
        arm_x11(title)
    after_request(title, lambda: activate("A"))
    if title == "X11GuardMinimize":
        wait_for(lambda: x11_wm_state(title) == 3, 60)
    state = window_state(title)
    minimized = window_minimized(title)
    print(f"{title} after inactive request: {state}, minimized {minimized}")
    if not state or not state.startswith("true|"):
        problems.append(f"{title} left fullscreen after its inactive request ({state})")
    if minimized:
        problems.append(f"{title} minimized after its inactive request")
    if title == "X11GuardMinimize" and x11_wm_state(title) != 3:
        problems.append(f"{title} did not receive the fake iconic acknowledgement")
    if title == "X11GuardMinimize":
        activate(title)
        wait_for(lambda: x11_wm_state(title) == 1, 60)
    else:
        after_request(title, lambda: activate(title))
    if title.endswith("GuardUnfullscreen"):
        wait_for(lambda: not is_fullscreen(title), 60)
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
    else:
        if title.startswith("X11Guard") and not minimize_x11(title):
            problems.append(f"{title} could not be found for its windowed minimize check")
        if not wait_for(lambda: window_minimized(title), 60):
            problems.append(f"{title} did not accept minimize after becoming windowed")


def reload_effect(titles, problems):
    if not reload_konveyor(titles):
        problems.append("the reloaded effect did not manage the windows that were already open")
    print("reloaded the effect with the windows already open")


def main():
    problems = []
    guard = str(build_dir() / "bin" / "fullscreen_guard_client")
    x11_guard = str(CLIENTS / "fullscreen-guard-x11-client.py")
    launch("GuardUnfullscreen", [guard, "GuardUnfullscreen", "unfullscreen"], {"WAYLAND_DEBUG": "client"})
    launch("GuardMinimize", [guard, "GuardMinimize", "minimize"], {"WAYLAND_DEBUG": "client"})
    launch("X11GuardUnfullscreen", ["python3", x11_guard, "X11GuardUnfullscreen", "unfullscreen"])
    launch("X11GuardMinimize", ["python3", x11_guard, "X11GuardMinimize", "minimize"])
    titles = ("GuardUnfullscreen", "GuardMinimize", "X11GuardUnfullscreen", "X11GuardMinimize")
    for title in titles:
        wait_for(lambda: is_fullscreen(title), 60)
    if sys.argv[1:] == ["reload"]:
        reload_effect(titles, problems)
    check_requests("GuardUnfullscreen", problems)
    check_requests("GuardMinimize", problems)
    check_requests("X11GuardMinimize", problems)
    check_requests("X11GuardUnfullscreen", problems)
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
