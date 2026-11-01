#!/usr/bin/env python3
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE.parent / "harness"), str(HERE)]

from checks import Checks, default_config, load_config
from fakepointer import Held, click, move
from kwinsession import activate, active_title, run_script, wait_for
from widgets_load import install, plasma

CLOCK = (1798, 1052)
OVER_C = (1250, 900)
ON_CALENDAR = (1500, 800)
CALENDAR = 'for (const w of workspace.stackingOrder) { if (!w.deleted && w.appletPopup) print("MARK|" + (w.active ? "active" : "inactive")); }'


def calendar():
    printed = run_script(CALENDAR)
    return printed[0] if printed else "gone"


def open_calendar():
    activate("B")
    for _ in range(3):
        click(*CLOCK)
        if wait_for(lambda: calendar() == "active", 4):
            return True
        activate("B")
    return False


def glide(held, start, end, steps, interval):
    for step in range(1, steps + 1):
        held.send(f"move:{start[0] + (end[0] - start[0]) * step / steps}:{start[1] + (end[1] - start[1]) * step / steps}")
        time.sleep(interval)


def panel_loads(checks):
    install(checks)
    checks.expect(wait_for(lambda: plasma("print(desktops().length);") not in ("", "0"), 180, 1), "plasmashell is up with a desktop")
    time.sleep(6)
    checks.expect(load_config(default_config().replace("    // focus-follows-mouse\n", "    focus-follows-mouse\n")), "focus-follows-mouse loads")


def hover_still_follows_without_a_popup(checks):
    activate("B")
    move(*OVER_C)
    checks.expect(wait_for(lambda: active_title() == "C", 5), f"hovering C focuses it when no popup is active ({active_title()})")


def calendar_survives_a_slow_crossing(checks):
    checks.expect(open_calendar(), "clicking the clock opens the calendar")
    with Held() as held:
        held.send(f"move:{CLOCK[0]}:{CLOCK[1]}")
        glide(held, CLOCK, OVER_C, 50, 0.04)
        time.sleep(0.8)
        glide(held, OVER_C, ON_CALENDAR, 20, 0.025)
        time.sleep(1.5)
    checks.equal(calendar(), "active", "the calendar stays open and active after the pointer dwelled on a tiled window")


def calendar_survives_hovering(checks):
    checks.expect(calendar() == "active" or open_calendar(), "the calendar is open")
    move(*ON_CALENDAR)
    with Held() as held:
        for step in range(30):
            held.send(f"move:{ON_CALENDAR[0] + (step % 5) * 6}:{ON_CALENDAR[1] + (step % 3) * 6}")
            time.sleep(0.1)
    checks.equal(calendar(), "active", "resting and jiggling on the calendar keeps it")


def clicking_the_tile_closes_it(checks):
    checks.expect(calendar() == "active" or open_calendar(), "the calendar is open")
    click(*OVER_C)
    checks.expect(wait_for(lambda: calendar() == "gone", 5), f"clicking a tiled window closes the calendar ({calendar()})")
    checks.expect(wait_for(lambda: active_title() == "C", 5), f"and focuses that window ({active_title()})")


def main():
    Checks().run(panel_loads, hover_still_follows_without_a_popup, calendar_survives_a_slow_crossing, calendar_survives_hovering, clicking_the_tile_closes_it)


if __name__ == "__main__":
    main()
