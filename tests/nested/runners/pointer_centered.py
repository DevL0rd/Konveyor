#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from fakepointer import move
from kwinsession import (
    activate,
    active_title,
    frame,
    konveyor_action,
    run_script,
    wait_for,
)


def cursor():
    x, y = run_script(
        'print("MARK|" + workspace.cursorPos.x + "|" + workspace.cursorPos.y);'
    )[0].split("|")
    return round(float(x)), round(float(y))


def under_pointer():
    return run_script(
        'const w = workspace.windowAt(workspace.cursorPos, 1)[0]; print("MARK|" + (w ? w.caption : "none"));'
    )[0]


def settle(title):
    previous = [None]

    def still():
        current = frame(title)
        done = current == previous[0] and current is not None
        previous[0] = current
        return done

    return wait_for(still, 10, 0.3)


def sizes():
    for title, width in (("A", "50%"), ("B", "20%"), ("C", "35%")):
        activate(title)
        konveyor_action("set-column-width", width)


def go(action, title):
    konveyor_action(action)
    return wait_for(lambda: active_title() == title, 10) and settle(title)


def report(label):
    print(
        f"INFO {label}: frames A={frame('A')} B={frame('B')} C={frame('C')} cursor={cursor()} under={under_pointer()}"
    )


def centered_scroll(checks):
    sizes()
    for mode, line in (
        ("off", ""),
        ("nearest", "    warp-mouse-to-focus\n"),
        ("center-xy", '    warp-mouse-to-focus mode="center-xy"\n'),
    ):
        checks.expect(
            load_config(
                default_config()
                .replace(
                    'center-focused-column "never"', 'center-focused-column "always"'
                )
                .replace("    // warp-mouse-to-focus\n", line)
            ),
            f"{mode} loads",
        )
        checks.expect(go("focus-column-first", "A"), "A focused")
        move(300, 300)
        checks.expect(go("focus-column-right", "B"), "B focused")
        report(f"{mode} narrow B centred, pointer was at 300,300")
        checks.expect(
            frame("B")[0] > 300 and abs(frame("B")[0] + frame("B")[2] / 2 - 960) <= 2,
            f"B is centred by its own width ({frame('B')})",
        )
        if mode == "off":
            checks.expect(
                under_pointer() != "B" and cursor() == (300, 300),
                f"off: the pointer stays where it was ({cursor()}, over {under_pointer()})",
            )
        else:
            checks.expect(
                wait_for(lambda: under_pointer() == "B", 5),
                f"{mode}: the pointer was brought onto B ({cursor()}, over {under_pointer()})",
            )
        if mode == "center-xy":
            checks.expect(
                cursor()
                == (
                    round(frame("B")[0] + frame("B")[2] / 2),
                    round(frame("B")[1] + frame("B")[3] / 2),
                ),
                f"center-xy: the pointer is at the centre of B ({cursor()})",
            )
        move(1700, 300)
        checks.expect(go("focus-column-left", "A"), "focus A")
        report(f"{mode} wide A, pointer was at 1700,300")
        if mode != "off":
            checks.expect(
                wait_for(lambda: under_pointer() == "A", 5),
                f"{mode}: the pointer was brought onto A ({cursor()}, over {under_pointer()})",
            )
    load_config(default_config())


def hover_after_scroll(checks):
    sizes()
    checks.expect(
        load_config(
            default_config()
            .replace('center-focused-column "never"', 'center-focused-column "always"')
            .replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")
        ),
        "ffm loads",
    )
    checks.expect(go("focus-column-first", "A"), "A focused")
    x, y, w, h = frame("A")
    move(round(x + w / 2), round(y + h / 2))
    checks.expect(go("focus-column-right", "B"), "B focused by keyboard")
    report("ffm after keyboard to B")
    checks.expect(
        not wait_for(lambda: active_title() != "B", 3),
        f"scrolling under a still pointer does not move focus ({active_title()})",
    )
    checks.expect(go("focus-column-right", "C"), "C focused by keyboard")
    checks.expect(
        not wait_for(lambda: active_title() != "C", 3), f"still C ({active_title()})"
    )
    load_config(default_config())


if __name__ == "__main__":
    Checks().run(centered_scroll, hover_after_scroll)
