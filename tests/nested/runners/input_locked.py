#!/usr/bin/env python3
import re
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, load_config
from input_signals import INPUT, LOGS, center, click_at, fired, observing, of, open_logged_client, press_a, probing_config
from kwinsession import activate, active_title, frame, wait_for
from lock_screen import screensaver


def settled(title, still=0.5):
    last = [None, time.monotonic()]

    def steady():
        current = frame(title)
        if current != last[0]:
            last[0], last[1] = current, time.monotonic()
        return time.monotonic() - last[1] >= still

    return wait_for(steady, 30, 0.1)


def pointer_inside(title):
    events = re.findall(r"wl_pointer#\d+\.(enter|leave)\(", LOGS[title].read_text(errors="replace"))
    return bool(events) and events[-1] == "enter"


def sideways_wheel_points_sideways(checks):
    checks.expect(load_config(probing_config()), "a config referencing input signals loads")
    checks.expect(wait_for(observing, 30), "the spy is installed")
    checks.expect(settled("B"), "B stops moving")
    x, y = center("B")
    INPUT.glide((x - 40, y), (x + 40, y), steps=8)
    checks.expect(wait_for(lambda: pointer_inside("B"), 30), "the pointer is inside B")
    steps = fired("B", "scroll.step")
    for _ in range(6):
        INPUT.send("axis:1:15")
    checks.expect(wait_for(lambda: fired("B", "scroll.step") == steps + 6, 30), "six horizontal notches fire six steps")
    checks.equal(of("B")["directionX"], 1, "the event direction points right")
    checks.equal(of("B")["directionY"], 0, "and not down")


def a_locked_screen_gets_no_input_signals(checks):
    activate("B")
    names = ("click", "scroll.step", "keypress")
    before = {name: fired("B", name) for name in names}
    screensaver("Lock")
    checks.expect(wait_for(lambda: screensaver("GetActive") == "true"), "the screen locks")
    x, y = center("B")
    click_at(x, y)
    for _ in range(3):
        INPUT.send("axis:0:15")
    press_a()
    active_title()
    time.sleep(0.5)
    for name in names:
        checks.equal(fired("B", name), before[name], f"{name} is not counted behind the lock screen")


def main():
    checks = Checks()
    with INPUT:
        processes = [open_logged_client("A", "text-client.qml"), open_logged_client("B", "client.qml")]
        try:
            checks.run(sideways_wheel_points_sideways, a_locked_screen_gets_no_input_signals)
        finally:
            for process in processes:
                process.terminate()


if __name__ == "__main__":
    main()
