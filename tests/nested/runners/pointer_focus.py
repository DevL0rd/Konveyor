#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from fakepointer import move
from kwinsession import active_title, frame, konveyor_action, run_script, wait_for

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


def main():
    Checks().run(warp_modes, focus_follows_mouse)


if __name__ == "__main__":
    main()
