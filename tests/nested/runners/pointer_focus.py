#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from fakepointer import Held, click, move
from kwinsession import CLIENTS, active_title, for_window, frame, konveyor_action, kwin_titles, run_script, wait_for

WARP = "    // warp-mouse-to-focus\n"
POINTER_OVER_B = (1400, 300)


def cursor():
    x, y = run_script('print("MARK|" + workspace.cursorPos.x + "|" + workspace.cursorPos.y);')[0].split("|")
    return round(float(x)), round(float(y))


def window_under_pointer():
    printed = run_script('const w = workspace.windowAt(workspace.cursorPos, 1)[0]; print("MARK|" + (w ? w.caption : "none"));')
    return printed[0]


def center(title):
    x, y, width, height = frame(title)
    return round(x + width / 2), round(y + height / 2)


def focus(action, title):
    konveyor_action(action)
    return wait_for(lambda: active_title() == title) and wait_for(lambda: frame(title)[0] == 16 or title != "A")


def warp_modes(checks):
    modes = {"off": "", "nearest": "    warp-mouse-to-focus\n", "center-xy": '    warp-mouse-to-focus mode="center-xy"\n',
             "center-xy-always": '    warp-mouse-to-focus mode="center-xy-always"\n'}
    for mode, line in modes.items():
        checks.expect(load_config(default_config().replace(WARP, line)), f"warp mode {mode} loads")
        checks.expect(focus("focus-column-first", "A"), "A is focused at the start of the row")
        move(*POINTER_OVER_B)
        checks.expect(focus("focus-column-right", "B"), "focus moves to B under the pointer")
        expected = center("B") if mode == "center-xy-always" else POINTER_OVER_B
        checks.expect(wait_for(lambda: cursor() == expected, 5), f"{mode}: focusing the window under the pointer leaves it at {expected} ({cursor()})")
        checks.expect(focus("focus-column-left", "A"), "focus moves back to A")
        if mode == "off":
            checks.expect(wait_for(lambda: cursor() == POINTER_OVER_B, 5), f"off: the pointer stays over B ({cursor()})")
        elif mode == "nearest":
            checks.expect(wait_for(lambda: cursor()[1] == POINTER_OVER_B[1] and window_under_pointer() == "A", 5),
                          f"nearest: the pointer moves sideways just onto A ({cursor()}, over {window_under_pointer()}, A at {frame('A')})")
        else:
            checks.expect(wait_for(lambda: cursor() == center("A"), 5), f"{mode}: the pointer jumps to the middle of A ({cursor()} vs {center('A')})")
    load_config(default_config())


def focus_follows_mouse(checks):
    checks.expect(load_config(default_config().replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    checks.expect(focus("focus-column-first", "A"), "A is focused")
    move(*POINTER_OVER_B)
    checks.expect(wait_for(lambda: active_title() == "B", 5), f"hovering B focuses it ({active_title()})")
    move(*center("A"))
    checks.expect(wait_for(lambda: active_title() == "A", 5), f"hovering A focuses it again ({active_title()})")
    load_config(default_config())
    move(*POINTER_OVER_B)
    checks.expect(not wait_for(lambda: active_title() == "B", 2), "without focus-follows-mouse hovering B leaves focus on A")


def popup_over_tiled_window(checks):
    checks.expect(load_config(default_config().replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    checks.expect(focus("focus-column-right", "B"), "B is focused")
    x, y, width, height = frame("A")
    inside = (round(x + width / 2), round(y + height / 2))
    popup = subprocess.Popen(["python3", str(CLIENTS / "x11-types.py"), "X-Popup:MENU"])
    try:
        checks.expect(wait_for(lambda: "X-Popup" in kwin_titles(), 30), "the popup mapped")
        run_script(for_window("X-Popup", f"w.frameGeometry = {{x: {inside[0] - 50}, y: {inside[1] - 50}, width: 100, height: 100}};"))
        checks.expect(wait_for(lambda: window_under_pointer_at(inside) == "X-Popup", 5), "the popup covers A")
        move(*inside)
        checks.expect(not wait_for(lambda: active_title() != "B", 2), f"hovering a popup over A keeps focus on B ({active_title()})")
    finally:
        popup.terminate()
    load_config(default_config())


def override_redirect_menu_over_tiled_window(checks):
    checks.expect(load_config(default_config().replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    checks.expect(focus("focus-column-first", "A"), "A is focused")
    checks.expect(focus("focus-column-right", "B"), "B is focused")
    popup = subprocess.Popen(["python3", str(CLIENTS / "x11-types.py"), "X-Menu:POPUP_MENU:override"])
    try:
        inside = (200, 200)
        checks.expect(wait_for(lambda: window_under_pointer_at(inside) != "A", 30), f"the override-redirect menu covers A ({window_under_pointer_at(inside)})")
        move(*inside)
        checks.expect(not wait_for(lambda: active_title() != "B", 2), f"hovering an override-redirect menu over A keeps focus on B ({active_title()})")
    finally:
        popup.terminate()
    load_config(default_config())


def applet_popup_over_tiled_window(checks):
    checks.expect(load_config(default_config().replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    checks.expect(focus("focus-column-first", "A"), "A is focused")
    popup = subprocess.Popen(["qml6", str(CLIENTS / "applet-popup-client.qml"), "--", "100", "100"])
    try:
        inside = (300, 250)
        checks.expect(wait_for(lambda: window_under_pointer_at(inside) != "A", 30), f"the applet popup covers A ({window_under_pointer_at(inside)})")
        move(*inside)
        checks.expect(not wait_for(lambda: active_title() == "A", 2), f"hovering an applet popup over A leaves the popup focused ({active_title()})")
    finally:
        popup.terminate()
    load_config(default_config())


def applet_popup_reached_across_tiled_window(checks):
    checks.expect(load_config(default_config().replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    checks.expect(focus("focus-column-first", "A"), "A is focused")
    popup = subprocess.Popen(["qml6", str(CLIENTS / "applet-popup-client.qml"), "--", "0", "0"])
    try:
        checks.expect(wait_for(lambda: "Qml Runtime" in kwin_titles(), 30), "the applet popup mapped")
        run_script(for_window("Qml Runtime", "w.frameGeometry = {x: 400, y: 500, width: 500, height: 500};"))
        checks.expect(wait_for(lambda: window_geometry("Qml Runtime") == "400,500 500x500", 5), "the applet popup sits over the lower part of A")
        checks.expect(wait_for(lambda: active_title() == "Qml Runtime", 5), "the applet popup is active")
        move(500, 1072)
        with Held() as held:
            held.glide((500, 1072), (500, 700), 40)
        checks.expect(not wait_for(lambda: "Qml Runtime" not in kwin_titles() or active_title() != "Qml Runtime", 2), f"sweeping across A to reach the popup keeps the popup ({active_title()})")
    finally:
        popup.terminate()
    load_config(default_config())


def window_geometry(title):
    printed = run_script(for_window(title, 'const g = w.frameGeometry; print("MARK|" + g.x + "," + g.y + " " + g.width + "x" + g.height);'))
    return printed[0] if printed else None


def window_under_pointer_at(point):
    printed = run_script(f'const w = workspace.windowAt({{x: {point[0]}, y: {point[1]}}}, 1)[0]; print("MARK|" + (w ? w.caption : "none"));')
    return printed[0]


def popup_geometry():
    printed = run_script('for (const w of workspace.stackingOrder) { if (w.popupWindow && !w.deleted) print("MARK|" + w.frameGeometry.x + "|" + w.frameGeometry.y + "|" + w.frameGeometry.width + "|" + w.frameGeometry.height); }')
    return [float(value) for value in printed[0].split("|")] if printed else None


def wayland_popup_over_neighbour(checks):
    checks.expect(load_config(default_config().replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    checks.expect(focus("focus-column-first", "A"), "A is focused")
    x, y, width, height = frame("A")
    origin = (round(x + width - 20), round(y + 100))
    click(*origin)
    checks.expect(wait_for(popup_geometry, 10), "the popup opened")
    px, py, pw, ph = popup_geometry()
    inside = (round(frame("B")[0] + 40), round(py + ph / 2))
    move(*inside)
    checks.expect(not wait_for(lambda: active_title() != "A", 3), f"hovering the popup over B keeps focus on A ({active_title()})")
    load_config(default_config())


def main():
    Checks().run(warp_modes, focus_follows_mouse, popup_over_tiled_window, override_redirect_menu_over_tiled_window, applet_popup_over_tiled_window, applet_popup_reached_across_tiled_window,
                  wayland_popup_over_neighbour)


if __name__ == "__main__":
    main()
