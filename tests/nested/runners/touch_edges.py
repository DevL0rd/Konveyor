#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from fakepointer import Held, binary
from kwinsession import (
    active_title,
    for_window,
    konveyor_action,
    konveyor_windows,
    managed_titles,
    run_script,
    wait_for,
)


def title_bar_geometry(title):
    printed = run_script(
        for_window(
            title,
            'print("MARK|" + w.frameGeometry.y + "|" + w.clientGeometry.y + "|" + w.frameGeometry.x + "|" + w.frameGeometry.width);',
        )
    )
    if not printed:
        return None
    frame_y, client_y, x, width = (float(value) for value in printed[0].split("|"))
    return frame_y, client_y, x, width


def laid_out(title, x, width):
    window = next((w for w in konveyor_windows() if w["title"] == title), None)
    if not window or not window["layout"]:
        return False
    target = window["layout"]["tile_pos_in_workspace_view"][0]
    size = window["layout"]["tile_size"][0]
    return abs(x - target) <= 1 and abs(width - size) <= 1


def title_bar_point(title):
    previous = [None]

    def still():
        current = title_bar_geometry(title)
        stable = (
            current is not None
            and current == previous[0]
            and current[1] > current[0]
            and laid_out(title, current[2], current[3])
        )
        previous[0] = current
        return current if stable else None

    found = wait_for(still, 60, 0.3)
    if not found:
        raise RuntimeError(f"the title bar of {title} never settled")
    frame_y, client_y, x, width = found
    return x + width / 3, (frame_y + client_y) / 2


def columns():
    return [
        title
        for _, title in sorted(
            (window["layout"]["tile_pos_in_workspace_view"][0], window["title"])
            for window in konveyor_windows()
        )
    ]


def interactive_moves():
    return run_script(
        'for (const w of workspace.windowList()) { if (!w.deleted && w.move) print("MARK|" + w.caption); }'
    )


def settle(checks, message):
    checks.expect(
        wait_for(lambda: not interactive_moves() and active_title() is not None),
        message,
    )


def scroll_view_title_bars():
    return default_config().replace(
        'titlebar-drag "move-window"', 'titlebar-drag "scroll-view"'
    )


def tap_without_moving(checks):
    checks.expect(
        load_config(scroll_view_title_bars()), "dragging a title bar scrolls the view"
    )
    before = columns()
    x, y = title_bar_point("A")
    with Held() as held:
        held.send(f"touchdown:0:{x}:{y}")
        checks.expect(
            wait_for(interactive_moves, 30), "touching the title bar starts a move"
        )
        held.send("touchup:0")
    settle(checks, "lifting the finger ends the move")
    checks.equal(columns(), before, "a tap on the title bar reorders nothing")


def jitter_then_drag(checks):
    x, y = title_bar_point("A")
    with Held() as held:
        held.send(f"touchdown:0:{x}:{y}", f"touchmotion:0:{x + 1}:{y}")
        held.send(*(f"touchmotion:0:{x + 20 + step * 25}:{y}" for step in range(12)))
        held.send("touchup:0")
    settle(checks, "the drag after a tiny jitter ends")


def window_closed_while_pending(checks):
    x, y = title_bar_point("C")
    with Held() as held:
        held.send(f"touchdown:0:{x}:{y}")
        checks.expect(wait_for(interactive_moves, 30), "the touch on C started a move")
        run_script(for_window("C", "w.closeWindow();"))
        checks.expect(
            wait_for(lambda: "C" not in managed_titles()), "C closed under the finger"
        )
        held.send(*(f"touchmotion:0:{x + 20 + step * 25}:{y}" for step in range(8)))
        held.send("touchup:0")
    settle(checks, "nothing keeps moving")


def second_finger_first(checks):
    x, y = title_bar_point("B")
    with Held() as held:
        held.send(f"touchdown:0:{x}:{y}", "touchdown:1:960:900")
        held.send(*(f"touchmotion:0:{x + 20 + step * 25}:{y}" for step in range(8)))
        held.send("touchup:0", "touchup:1")
    settle(checks, "a move begun with two fingers down is cancelled")
    checks.expect(konveyor_action("focus-column-first") == "", "layout still answers")


def drag_without_long_press(checks):
    base = scroll_view_title_bars()
    for label, wanted, unwanted in (
        (
            "long press to move off",
            "        long-press-to-move false\n",
            "        long-press-to-move\n",
        ),
        (
            "the touchscreen off",
            "        off\n        long-press-to-move\n",
            "        long-press-to-move\n",
        ),
    ):
        text = base.replace(unwanted, wanted)
        checks.expect(text != base and load_config(text), f"{label} loads")
        x, y = title_bar_point("A")
        with Held() as held:
            held.send(f"touchdown:0:{x}:{y}")
            held.send(
                *(f"touchmotion:0:{x + 20 + step * 25}:{y}" for step in range(10))
            )
            held.send("touchup:0")
        settle(checks, f"the drag ends with {label}")
        checks.equal(managed_titles(), ["A", "B"], f"nothing is lost with {label}")
    load_config(base)


def second_finger_joins_a_title_bar_drag(checks):
    x, y = title_bar_point("A")
    with Held() as held:
        held.send(f"touchdown:0:{x}:{y}")
        held.send(*(f"touchmotion:0:{x + 20 + step * 25}:{y}" for step in range(6)))
        held.send("touchdown:1:960:900")
        held.send(*(f"touchmotion:0:{x + 170 + step * 25}:{y}" for step in range(6)))
        held.send("touchup:1", "touchup:0")
    settle(checks, "the title bar drag ends after a second finger joined it")
    checks.equal(managed_titles(), ["A", "B"], "no window is lost")


def main():
    binary()
    Checks().run(
        tap_without_moving,
        jitter_then_drag,
        second_finger_first,
        window_closed_while_pending,
        second_finger_joins_a_title_bar_drag,
        drag_without_long_press,
    )


if __name__ == "__main__":
    main()
