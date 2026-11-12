#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from fakepointer import Held, click, move
from kwinsession import CLIENTS, activate, active_title, for_window, frame, frames, konveyor_action, kwin_titles, open_client, run_script, wait_for, window_minimized

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
        checks.expect(load_config(default_config(wrapped=False).replace(WARP, line)), f"warp mode {mode} loads")
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
    load_config(default_config(wrapped=False))


def warp_from_outside_the_frame(checks):
    checks.expect(load_config(default_config(wrapped=False).replace(WARP, "    warp-mouse-to-focus\n")), "warp-mouse-to-focus loads")
    for label, start in (("left of", (100, 300)), ("above", (1400, 5)), ("below", (1400, 1075))):
        checks.expect(focus("focus-column-first", "A"), "A is focused")
        move(*start)
        checks.expect(focus("focus-column-right", "B"), f"focus moves to B with the pointer {label} it")
        checks.expect(wait_for(lambda: window_under_pointer() == "B", 5), f"the pointer was brought onto B from {label} it ({cursor()})")
    load_config(default_config(wrapped=False))


def focus_follows_mouse(checks):
    checks.expect(load_config(default_config(wrapped=False).replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    checks.expect(focus("focus-column-first", "A"), "A is focused")
    move(*POINTER_OVER_B)
    checks.expect(wait_for(lambda: active_title() == "B", 5), f"hovering B focuses it ({active_title()})")
    move(*center("A"))
    checks.expect(wait_for(lambda: active_title() == "A", 5), f"hovering A focuses it again ({active_title()})")
    load_config(default_config(wrapped=False))
    move(*POINTER_OVER_B)
    checks.expect(not wait_for(lambda: active_title() == "B", 2), "without focus-follows-mouse hovering B leaves focus on A")


def popup_over_tiled_window(checks):
    checks.expect(load_config(default_config(wrapped=False).replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
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
    load_config(default_config(wrapped=False))


def override_redirect_menu_over_tiled_window(checks):
    checks.expect(load_config(default_config(wrapped=False).replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
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
    load_config(default_config(wrapped=False))


def applet_popup_over_tiled_window(checks):
    checks.expect(load_config(default_config(wrapped=False).replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    checks.expect(focus("focus-column-first", "A"), "A is focused")
    popup = subprocess.Popen(["qml6", str(CLIENTS / "applet-popup-client.qml"), "--", "100", "100"])
    try:
        inside = (300, 250)
        checks.expect(wait_for(lambda: window_under_pointer_at(inside) != "A", 30), f"the applet popup covers A ({window_under_pointer_at(inside)})")
        move(*inside)
        checks.expect(not wait_for(lambda: active_title() == "A", 2), f"hovering an applet popup over A leaves the popup focused ({active_title()})")
    finally:
        popup.terminate()
    load_config(default_config(wrapped=False))


def applet_popup_reached_across_tiled_window(checks):
    checks.expect(load_config(default_config(wrapped=False).replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
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
    load_config(default_config(wrapped=False))


def window_geometry(title):
    printed = run_script(for_window(title, 'const g = w.frameGeometry; print("MARK|" + g.x + "," + g.y + " " + g.width + "x" + g.height);'))
    return printed[0] if printed else None


def window_under_pointer_at(point):
    printed = run_script(f'const w = workspace.windowAt({{x: {point[0]}, y: {point[1]}}}, 1)[0]; print("MARK|" + (w ? w.caption : "none"));')
    return printed[0]


def popup_geometry():
    printed = run_script('for (const w of workspace.stackingOrder) { if (w.popupWindow && !w.deleted) print("MARK|" + w.frameGeometry.x + "|" + w.frameGeometry.y + "|" + w.frameGeometry.width + "|" + w.frameGeometry.height); }')
    return [float(value) for value in printed[0].split("|")] if printed else None


def stable_frame(title):
    previous = [None]

    def still():
        current = frame(title)
        stable = current == previous[0]
        previous[0] = current
        return stable

    return still


def shown_windows():
    return sorted(title for title, (x, y, width, height) in frames().items() if title in ("A", "B", "C") and 0 <= x < 1920 and width > 0)


def narrow_columns():
    for title in ("A", "B", "C"):
        activate(title)
        konveyor_action("set-column-width", "30%")
    activate("A")


def hovering_with_nothing_active(checks):
    narrow_columns()
    checks.expect(load_config(default_config(wrapped=False).replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    target = shown_windows()[0]
    run_script("workspace.activeWindow = null;")
    checks.expect(wait_for(lambda: active_title() == "none"), "no window is active")
    move(*center(target))
    checks.expect(wait_for(lambda: active_title() == target, 10), f"hovering {target} focuses it although nothing was active ({active_title()})")
    load_config(default_config(wrapped=False))


def hovered_window_closed_or_focused_before_the_delay(checks):
    checks.expect(load_config(default_config(wrapped=False).replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    first, second = shown_windows()[:2]
    activate(first)
    move(*center(second))
    run_script(for_window(second, "w.closeWindow();"))
    checks.expect(wait_for(lambda: second not in kwin_titles()), f"{second} closed while it was waiting to be focused")
    checks.expect(not wait_for(lambda: active_title() == second, 2), "nothing is focused for the closed window")
    open_client(second, client="popup-client.qml")
    third = next(title for title in shown_windows() if title != first)
    activate(first)
    checks.expect(wait_for(stable_frame(third), 10, 0.3), f"{third} stops moving")
    move(*center(third))
    activate(third)
    checks.expect(not wait_for(lambda: active_title() != third, 2), f"the pending hover does not take focus back ({active_title()})")
    load_config(default_config(wrapped=False))


def hover_other_window(checks, hovered, expected_active, message):
    checks.expect(wait_for(stable_frame(hovered), 10, 0.3), f"{hovered} stops moving")
    move(*center(hovered))
    checks.expect(wait_for(lambda: active_title() == expected_active, 10), f"{message} ({active_title()})")


def windows_that_are_not_shown_above_the_target(checks):
    checks.expect(load_config(default_config(wrapped=False).replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    checks.expect(focus("focus-column-first", "A"), "A is focused")
    run_script(for_window("A", "w.keepAbove = true; w.minimized = true;"))
    checks.expect(wait_for(lambda: window_minimized("A") and active_title() != "A"), "A is minimized but still above the others")
    other = "B" if active_title() == "C" else "C"
    hover_other_window(checks, other, other, f"a minimized window above {other} does not stop hovering it focusing it")
    run_script(for_window("A", "w.minimized = false; w.keepAbove = false;"))
    checks.expect(wait_for(lambda: not window_minimized("A")), "A is back")
    run_script(for_window("B", "w.keepAbove = true;"))
    checks.expect(activate("B"), "B is focused")
    checks.equal(konveyor_action("move-window-to-workspace", "2"), "", "send B to the second workspace")
    checks.equal(konveyor_action("focus-workspace", "1"), "", "come back to the first workspace")
    checks.expect(wait_for(lambda: active_title() in ("A", "C")), "a window on the first workspace is active")
    target = "A" if active_title() == "C" else "C"
    hover_other_window(checks, target, target, f"a window on another desktop above {target} does not stop hovering it focusing it")
    load_config(default_config(wrapped=False))


def wayland_popup_over_neighbour(checks):
    checks.expect(load_config(default_config(wrapped=False).replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")
    checks.expect(focus("focus-column-first", "A"), "A is focused")
    x, y, width, height = frame("A")
    origin = (round(x + width - 20), round(y + 100))
    click(*origin)
    checks.expect(wait_for(popup_geometry, 10), "the popup opened")
    px, py, pw, ph = popup_geometry()
    inside = (round(frame("B")[0] + 40), round(py + ph / 2))
    move(*inside)
    checks.expect(not wait_for(lambda: active_title() != "A", 3), f"hovering the popup over B keeps focus on A ({active_title()})")
    load_config(default_config(wrapped=False))


def main():
    Checks().run(warp_modes, warp_from_outside_the_frame, focus_follows_mouse, popup_over_tiled_window, override_redirect_menu_over_tiled_window, applet_popup_over_tiled_window, applet_popup_reached_across_tiled_window,
                  wayland_popup_over_neighbour, windows_that_are_not_shown_above_the_target, hovering_with_nothing_active,
                  hovered_window_closed_or_focused_before_the_delay)


if __name__ == "__main__":
    main()
