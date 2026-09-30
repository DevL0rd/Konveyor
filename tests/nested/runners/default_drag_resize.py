#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, config_path
from dragging import LEFT_BUTTON, settled
from fakepointer import Held
from kwinsession import frame, konveyor, konveyor_action, konveyor_windows, run_script, wait_for

GROW = 120
RESIZE_MARGIN = 3


def order():
    placed = sorted((window["layout"]["tile_pos_in_workspace_view"][0], window["title"]) for window in konveyor_windows()
                    if (window.get("layout") or {}).get("tile_pos_in_workspace_view"))
    return [title for _, title in placed]


def title_bar_height(title):
    printed = run_script(f'for (const w of workspace.windowList()) {{ if (w.caption == "{title}") {{ print("MARK|" + w.frameGeometry.y + "|" + w.clientGeometry.y); }} }}')
    frame_y, client_y = (float(value) for value in printed[0].split("|"))
    return frame_y, client_y


def left_drag(start, end):
    with Held() as pointer:
        pointer.send(f"move:{start[0]}:{start[1]}", f"button:{LEFT_BUTTON}:1")
        pointer.glide(start, end, steps=24)
        pointer.send(f"button:{LEFT_BUTTON}:0")


def narrow_columns():
    konveyor_action("focus-column-first")
    for _ in range(3):
        konveyor_action("set-column-width", "30%")
        konveyor_action("focus-column-right")
    konveyor_action("focus-column-first")


def title_bar_drag_moves_the_window(checks):
    narrow_columns()
    checks.expect(settled("A"), "A settles before its title bar is dragged")
    before = order()
    checks.equal(before, ["A", "B", "C"], "A, B and C start in that order")
    frame_y, client_y = title_bar_height("A")
    checks.expect(client_y - frame_y >= 10, f"A has a title bar to drag (frame y {frame_y}, client y {client_y})")
    x, _, width, _ = frame("A")
    c_x, _, c_width, _ = frame("C")
    start = (x + width / 2, (frame_y + client_y) / 2)
    left_drag(start, (c_x + c_width * 0.75, start[1]))
    checks.expect(wait_for(lambda: order()[-1] == "A", 30), f"dragging A's title bar onto C's far side moves A to the end of the row ({before} -> {order()})")
    checks.equal(sorted(order()), ["A", "B", "C"], "every window is still tiled in the row")


def drag_right_border(title):
    x, y, width, height = frame(title)
    start = (x + width + RESIZE_MARGIN, y + height / 2)
    left_drag(start, (start[0] + GROW, start[1]))
    return width


def border_drag_resizes_the_column(checks):
    target = order()[0]
    checks.expect(settled(target), f"{target} settles before its border is dragged")
    width = drag_right_border(target)
    grown = wait_for(lambda: settled(target) and frame(target)[2] >= width + GROW - 10, 30)
    checks.expect(grown, f"dragging {target}'s right border by {GROW}px widens its column (from {width} to {frame(target)[2]})")


def border_drag_does_nothing_with_resizing_off(checks):
    text = config_path().read_text()
    config_path().write_text(text.replace("resize-tiled-windows true", "resize-tiled-windows false"))
    checks.equal(konveyor("LoadConfigFile", ""), "", "turn off resizing tiled windows")
    target = order()[0]
    checks.expect(settled(target), f"{target} settles before its border is dragged again")
    width = drag_right_border(target)
    checks.expect(settled(target), f"{target} settles after the drag")
    checks.equal(frame(target)[2], width, f"with resize-tiled-windows false, dragging {target}'s border leaves its width alone")


def main():
    Checks().run(title_bar_drag_moves_the_window, border_drag_resizes_the_column, border_drag_does_nothing_with_resizing_off)


if __name__ == "__main__":
    main()
