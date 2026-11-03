#!/usr/bin/env python3
import sys
import time
from pathlib import Path

from Xlib import X, Xatom, Xutil, display
from Xlib.protocol import event

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
import json

from kwinsession import activate, active_title, for_window, konveyor, konveyor_action, konveyor_windows, kwin_titles, managed_titles, qdbus, run_script, wait_for, window_minimized, window_state

EXPERIMENTS = "\nexperiments {\n    prevent-fullscreen-minimize\n    prevent-fullscreen-exit\n}\n"
ICONIC = 3
NORMAL = 1
REDIRECT = X.SubstructureRedirectMask | X.SubstructureNotifyMask

conn = display.Display()
root = conn.screen().root
ATOM = {name: conn.intern_atom(name) for name in ("_NET_WM_STATE", "_NET_WM_STATE_FULLSCREEN", "WM_CHANGE_STATE", "_NET_ACTIVE_WINDOW", "WM_STATE",
                                                    "_NET_WM_STATE_MAXIMIZED_VERT", "_NET_WM_STATE_MAXIMIZED_HORZ")}


def open_window(title):
    window = root.create_window(0, 0, 640, 480, 0, conn.screen().root_depth, X.InputOutput, X.CopyFromParent,
                                background_pixel=conn.screen().white_pixel, event_mask=X.PropertyChangeMask | X.FocusChangeMask)
    window.set_wm_name(title)
    window.set_wm_class(title.lower(), title)
    window.set_wm_hints(flags=Xutil.InputHint | Xutil.StateHint, input=1, initial_state=Xutil.NormalState)
    window.map()
    conn.flush()
    wait_for(lambda: title in kwin_titles(), 60)
    return window


def send(window, kind, data, size=32):
    root.send_event(event.ClientMessage(window=window, client_type=kind, data=(size, data)), event_mask=REDIRECT)
    conn.flush()


def set_fullscreen(window, action):
    send(window, ATOM["_NET_WM_STATE"], [action, ATOM["_NET_WM_STATE_FULLSCREEN"], 0, 1, 0])


def request_iconic(window):
    send(window, ATOM["WM_CHANGE_STATE"], [ICONIC, 0, 0, 0, 0])


def wm_state(window):
    state = window.get_full_property(ATOM["WM_STATE"], ATOM["WM_STATE"])
    return int(state.value[0]) if state is not None and len(state.value) else None


def is_fullscreen(title):
    return (window_state(title) or "").startswith("true|")


def hints(window):
    reply = window.get_wm_hints()
    return (reply.input, reply.initial_state) if reply else None


def settle(seconds=0.6):
    conn.sync()
    kwin_titles()
    time.sleep(seconds)


def send_to_background():
    activate("A")
    settle(0.5)


def setup(checks):
    global game, plain
    plain = open_window("EdgePlain")
    game = open_window("EdgeGame")
    set_fullscreen(game, 1)
    checks.expect(wait_for(lambda: is_fullscreen("EdgeGame"), 60), "the X11 game went fullscreen")
    checks.expect(activate("A"), "A is active so the game is in the background")


def stray_requests(checks):
    stray = root.create_window(0, 0, 100, 100, 0, conn.screen().root_depth, X.InputOutput, X.CopyFromParent)
    stray.configure(width=300, height=200, stack_mode=X.Above)
    request_iconic(stray)
    send(game, ATOM["_NET_WM_STATE"], [1] * 20, size=8)
    game.configure(x=10, y=20, width=500, height=400, stack_mode=X.Above)
    game.configure(width=700)
    game.configure(height=500)
    send(game, ATOM["WM_CHANGE_STATE"], [NORMAL, 0, 0, 0, 0])
    settle()
    checks.expect(is_fullscreen("EdgeGame") and window_minimized("EdgeGame") is False, "requests for unknown windows, odd formats and normal-state changes leave the game alone")
    checks.equal(active_title(), "A", "and focus stays on A")


def minimize_without_fullscreen(checks):
    request_iconic(plain)
    checks.expect(wait_for(lambda: window_minimized("EdgePlain")), "a window that is not fullscreen can be minimized")


def window_with_odd_state_property(checks):
    odd = open_window("EdgeOdd")
    odd.change_property(ATOM["_NET_WM_STATE"], Xatom.ATOM, 8, b"odd")
    request_iconic(odd)
    checks.expect(wait_for(lambda: window_minimized("EdgeOdd")), "a window whose state property has the wrong format is just minimized")
    odd.destroy()
    conn.flush()


def workspace_index(title):
    workspaces = {workspace["id"]: workspace["idx"] for workspace in json.loads(konveyor("Workspaces"))}
    return next((workspaces[window["workspace_id"]] for window in konveyor_windows() if window["title"] == title), None)


def desktop_follows_after_minimize(checks):
    window = open_window("EdgeDesk")
    checks.expect(wait_for(lambda: "EdgeDesk" in managed_titles()), "the window is tiled")
    run_script(for_window("EdgeDesk", "w.minimized = true;"))
    checks.expect(wait_for(lambda: "EdgeDesk" not in managed_titles()), "minimizing it takes it out of the layout")
    run_script(for_window("EdgeDesk", "w.minimized = false;"))
    checks.expect(wait_for(lambda: "EdgeDesk" in managed_titles()), "restoring it puts it back")
    window.set_wm_hints(flags=Xutil.InputHint, input=1)
    checks.expect(activate("EdgeDesk"), "the window is active")
    checks.equal(konveyor_action("toggle-window-floating"), "", "it floats")
    checks.expect(wait_for(lambda: next((w["is_floating"] for w in konveyor_windows() if w["title"] == "EdgeDesk"), False)), "and the layout agrees")
    settle(2.0)
    run_script(for_window("EdgeDesk", "w.frameGeometry = {x: 300, y: 300, width: 400, height: 300};"))
    checks.expect(wait_for(lambda: (window_state("EdgeDesk") or "").endswith("400x300"), 30), f"a floating window keeps the size it is resized to ({window_state('EdgeDesk')})")
    first = workspace_index("EdgeDesk")
    run_script(for_window("EdgeDesk", "w.desktops = [workspace.desktops[workspace.desktops.length - 1]];"))
    checks.expect(wait_for(lambda: workspace_index("EdgeDesk") not in (None, first)), "moving the restored window to another desktop moves it to that workspace")
    window.destroy()
    conn.flush()


def maximum_size_limits_the_window(checks):
    limited = open_window("EdgeLimited")
    limited.set_wm_normal_hints(flags=Xutil.PMaxSize, max_width=500, max_height=400)
    conn.flush()
    checks.expect(wait_for(lambda: (window_state("EdgeLimited") or "").endswith("x400") or " 500x" in (window_state("EdgeLimited") or ""), 30),
                  f"a window that sets a maximum size is made no larger ({window_state('EdgeLimited')})")
    limited.destroy()
    conn.flush()


def rule_unmanages_a_window(checks):
    steam = root.create_window(0, 0, 640, 480, 0, conn.screen().root_depth, X.InputOutput, X.CopyFromParent, background_pixel=conn.screen().white_pixel)
    steam.set_wm_name("Steam")
    steam.set_wm_class("steam", "steam")
    steam.map()
    conn.flush()
    checks.expect(wait_for(lambda: "Steam" in managed_titles(), 60), "an ordinary Steam window is tiled")
    steam.set_wm_name("SteamWebhelper")
    conn.flush()
    checks.expect(wait_for(lambda: "SteamWebhelper" in kwin_titles() and "SteamWebhelper" not in managed_titles(), 30), "a title matching a manage false rule hands the window back to KWin")
    steam.set_wm_name("Steam")
    conn.flush()
    checks.expect(wait_for(lambda: "Steam" in managed_titles(), 30), "and a title that stops matching brings it back")
    steam.destroy()
    conn.flush()


def hints_on_a_minimized_window(checks):
    plain.set_wm_normal_hints(flags=Xutil.PMinSize, min_width=300, min_height=200)
    settle()
    checks.expect(window_minimized("EdgePlain"), "size hints changing on a minimized window leave it minimized")
    plain.destroy()
    conn.flush()
    checks.expect(wait_for(lambda: "EdgePlain" not in kwin_titles(), 30), "a minimized window can be closed")


def maximize_request(checks):
    window = open_window("EdgeMax")
    maximized = [ATOM["_NET_WM_STATE_MAXIMIZED_VERT"], ATOM["_NET_WM_STATE_MAXIMIZED_HORZ"]]
    send(window, ATOM["_NET_WM_STATE"], [1, *maximized, 1, 0])
    checks.expect(wait_for(lambda: (window_state("EdgeMax") or "").endswith("1888x1048"), 30), f"a maximize request makes the window fill the width ({window_state('EdgeMax')})")
    window.destroy()
    conn.flush()


def activation_requests(checks):
    window = open_window("EdgeAct")
    for data in ([2, 1, 0, 0, 0], [2, 0, 0, 1, 0], [2, 0, 0, 0, 1], [2, 0, 0, 0, 0]):
        send(window, ATOM["_NET_ACTIVE_WINDOW"], data)
        activate("A")
        send(window, ATOM["_NET_ACTIVE_WINDOW"], data)
        settle(0.4)
    checks.expect(wait_for(lambda: active_title() == "EdgeAct", 20), "pager activation requests for a window that is not pretending are honoured")
    window.destroy()
    conn.flush()


def active_fullscreen_minimize(checks):
    activate("EdgeGame")
    request_iconic(game)
    checks.expect(wait_for(lambda: wm_state(game) == NORMAL, 20), "an active fullscreen game is told it is in the normal state when it asks to minimize")
    checks.expect(is_fullscreen("EdgeGame") and window_minimized("EdgeGame") is False, "and it stays fullscreen")


def pretend_minimize(checks):
    send_to_background()
    activate("EdgeGame")
    send_to_background()
    request_iconic(game)
    request_iconic(game)
    checks.expect(wait_for(lambda: wm_state(game) == ICONIC, 20), "a background fullscreen game believes it is minimized")
    checks.expect(window_minimized("EdgeGame") is False, "while KWin keeps it up")


def ignored_activation(checks):
    send(game, ATOM["_NET_ACTIVE_WINDOW"], [2, 0, 0, 0, 0])
    game.set_input_focus(X.RevertToParent, X.CurrentTime)
    settle(1.0)
    checks.equal(active_title(), "A", "an activation request or focus grab from a pretend-minimized game does not steal focus")
    checks.equal(wm_state(game), ICONIC, "and it still believes it is minimized")


def repair_hints(checks):
    wm_hints = Xatom.WM_HINTS
    for words in ([1, 0, 0, 0, 0, 0, 0, 0, 0], [2, 0, 1, 0, 0, 0, 0, 0, 0], [3, 1, 1, 0, 0, 0, 0, 0, 0]):
        game.change_property(wm_hints, wm_hints, 32, words)
        conn.flush()
        settle(0.3)
    game.change_property(wm_hints, wm_hints, 32, [1, 0])
    conn.flush()
    settle(0.3)
    game.change_property(wm_hints, wm_hints, 8, b"odd")
    conn.flush()
    settle(0.3)
    game.set_wm_hints(flags=Xutil.InputHint | Xutil.StateHint, input=0, initial_state=Xutil.IconicState)
    conn.flush()
    checks.expect(wait_for(lambda: hints(game) == (1, NORMAL), 20), f"hints that would stop the game from taking focus are repaired ({hints(game)})")
    request_iconic(game)
    settle()
    checks.expect(wait_for(lambda: wm_state(game) == ICONIC, 20), "a repeated minimize request after the repair keeps the game pretending")


def wake_up(checks):
    activate("EdgeGame")
    checks.expect(wait_for(lambda: wm_state(game) == NORMAL, 20), "focusing the game ends the pretend minimize")
    settle(1.0)


def unload_while_pretending(checks):
    spare = open_window("EdgeSpare")
    keep = open_window("EdgeKeep")
    set_fullscreen(spare, 1)
    checks.expect(wait_for(lambda: is_fullscreen("EdgeSpare"), 60), "a second game went fullscreen")
    send_to_background()
    request_iconic(spare)
    checks.expect(wait_for(lambda: wm_state(spare) == ICONIC, 20), "the second game pretends to be minimized")
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.unloadEffect", "konveyor_effect")
    checks.expect(wait_for(lambda: wm_state(spare) == NORMAL, 20), "unloading Konveyor wakes a game it made pretend to be minimized")
    send(keep, ATOM["_NET_WM_STATE"], [1, ATOM["_NET_WM_STATE_MAXIMIZED_VERT"], ATOM["_NET_WM_STATE_MAXIMIZED_HORZ"], 1, 0])
    settle()
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.loadEffect", "konveyor_effect")
    checks.expect(wait_for(lambda: is_fullscreen("EdgeGame") and is_fullscreen("EdgeSpare") and "EdgeKeep" in managed_titles(), 30),
                  "the games are still fullscreen and the maximized window is tiled after Konveyor loads again")
    spare.destroy()
    keep.destroy()
    conn.flush()


def experiments_off(checks):
    send_to_background()
    request_iconic(game)
    checks.expect(wait_for(lambda: wm_state(game) == ICONIC, 20), "the game pretends to be minimized once more")
    checks.expect(load_config(default_config()), "the config without the experiments loads")
    checks.expect(wait_for(lambda: wm_state(game) == NORMAL, 20), "turning the experiments off wakes the pretending game")
    request_iconic(game)
    checks.expect(wait_for(lambda: window_minimized("EdgeGame")), "without the experiments a fullscreen game can minimize")
    free = open_window("EdgeFree")
    set_fullscreen(free, 1)
    checks.expect(wait_for(lambda: is_fullscreen("EdgeFree"), 60), "a new game goes fullscreen")
    free.set_wm_hints(flags=Xutil.InputHint, input=1)
    conn.flush()
    settle()
    send_to_background()
    request_iconic(free)
    checks.expect(wait_for(lambda: window_minimized("EdgeFree")), "and it minimizes when asked because nothing is preventing it")
    free.destroy()
    conn.flush()


def leave_fullscreen(checks):
    load_config(default_config() + EXPERIMENTS)
    root.send_event(event.ClientMessage(window=game, client_type=ATOM["_NET_ACTIVE_WINDOW"], data=(32, [1, 0, 0, 0, 0])), event_mask=REDIRECT)
    conn.flush()
    checks.expect(wait_for(lambda: window_minimized("EdgeGame") is False, 30), "the game was restored")
    activate("EdgeGame")
    set_fullscreen(game, 2)
    checks.expect(wait_for(lambda: not is_fullscreen("EdgeGame"), 30), "an active game can toggle itself out of fullscreen")
    set_fullscreen(game, 2)
    checks.expect(wait_for(lambda: is_fullscreen("EdgeGame"), 30), "and toggle back in")
    send(game, ATOM["_NET_WM_STATE"], [0, 0, ATOM["_NET_WM_STATE_FULLSCREEN"], 1, 0])
    checks.expect(wait_for(lambda: not is_fullscreen("EdgeGame"), 30), "the fullscreen atom can come in the second slot of the request")


def main():
    Checks().run(setup, stray_requests, minimize_without_fullscreen, window_with_odd_state_property, hints_on_a_minimized_window,
                 desktop_follows_after_minimize, maximum_size_limits_the_window, rule_unmanages_a_window,
                 maximize_request, activation_requests, active_fullscreen_minimize, pretend_minimize, ignored_activation, repair_hints,
                 wake_up, unload_while_pretending, experiments_off, leave_fullscreen)


if __name__ == "__main__":
    main()
