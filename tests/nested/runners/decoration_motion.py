#!/usr/bin/env python3
import sys
import threading
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from borderprobe import CYAN, MAGENTA, TITLES, YELLOW, edge_pixels, expect_painted, painted, painted_titles, probe_config, screenshot, settle_to, solid
from checks import Checks, load_config
from fakepointer import Held
from kwinsession import frame, wait_for

KEY_A = 30
LEFT_BUTTON = 0x110
FRONT_LEAST = 128
WAVE = ('border-style "Sweep" {\n    layer "gradient" { colors "#00ffff"; speed 0; width 8; }\n    layer "wave" { colors "#ff00ff"; speed 0.05; width 8; }\n'
        '    layer "ripple" { colors "#ffff00"; speed 0.05; width 8; }\n}\n')
BURST = ('border-style "Burst" {\n    layer "pulse-front" { colors "#ff00ff"; speed 0.03; width 8; }\n'
         '    layer "ripple" { colors "#ffff00"; speed 0.1; width 8; }\n}\n')


def center(title):
    x, y, width, height = frame(title)
    return x + width / 2, y + height / 2




def front_passing(image, title):
    return any(red >= FRONT_LEAST and blue >= FRONT_LEAST and green < FRONT_LEAST / 2 for red, green, blue in edge_pixels(image, title))


def light_front_crosses_the_window_in_between(checks):
    checks.expect(load_config(probe_config('        on "focus" "Sweep" reach="self" duration=40', WAVE)), "a wave effect loads")
    settle_to("A")
    settle_to("C")
    checks.expect(wait_for(lambda: painted(screenshot(), "C", CYAN), 15, 0.3), "the window that gained focus stands in the standing colour")
    crossed = False
    standing_elsewhere = False
    ripple_elsewhere = False
    deadline = time.monotonic() + 40
    while time.monotonic() < deadline and not crossed:
        image = screenshot()
        standing_elsewhere = standing_elsewhere or painted(image, "B", CYAN) or painted(image, "A", CYAN)
        ripple_elsewhere = ripple_elsewhere or painted(image, "B", YELLOW) or painted(image, "A", YELLOW)
        crossed = painted(image, "B", MAGENTA)
    checks.expect(crossed, "the wave front passes over the window between the two")
    checks.expect(not standing_elsewhere, "and the windows the front only crosses never show the standing colour")
    checks.expect(not ripple_elsewhere, "or the ripple, which only the focused window plays")


def a_click_sends_a_front_over_every_window(checks):
    checks.expect(load_config(probe_config('        on "click" "Burst" reach="all"', BURST)), "a click effect with a pulse front loads")
    settle_to("B")
    x, y = center("B")
    with Held() as pointer:
        pointer.send(f"move:{x}:{y}")
        time.sleep(0.3)
        pointer.send(f"button:{LEFT_BUTTON}:1", f"button:{LEFT_BUTTON}:0")
    seen = set()
    ripples = set()
    deadline = time.monotonic() + 30
    while time.monotonic() < deadline and seen != set(TITLES):
        image = screenshot()
        seen |= {title for title in TITLES if front_passing(image, title)}
        ripples |= painted_titles(image, TITLES, YELLOW)
    checks.equal(seen, set(TITLES), "the front sweeps over all three windows")
    checks.expect(len(ripples) > 0, f"and the ripple reaches some of them ({sorted(ripples)})")


class Feeder(threading.Thread):
    def __init__(self, pointer, events, pause):
        super().__init__()
        self.pointer = pointer
        self.events = events
        self.pause = pause
        self.running = True

    def run(self):
        while self.running:
            self.pointer.send(*self.events)
            time.sleep(self.pause)

    def stop(self):
        self.running = False
        self.join()


def while_mode_lasts_as_long_as_the_input(checks, events, trigger, expected, what):
    checks.expect(load_config(probe_config(f'        on "{trigger}" "Mark" reach="all" mode="while"', solid("Mark", "#ff00ff"))),
                  f"a while-mode {trigger} effect loads")
    settle_to("A")
    with Held() as pointer:
        x, y = center("B")
        pointer.send(f"move:{x}:{y}")
        time.sleep(0.3)
        settle_to("A")
        feeder = Feeder(pointer, events, 0.2)
        feeder.start()
        try:
            expect_painted(checks, MAGENTA, expected, f"{what} plays the effect for as long as it goes on")
        finally:
            feeder.stop()
    expect_painted(checks, MAGENTA, set(), f"and it ends shortly after {what} stops", 15)


def while_mode_repeats_one_shot_layers(checks):
    quick = 'border-style "Quick" {\n    layer "ripple" { colors "#ffff00"; speed 1; width 8; }\n}\n'
    checks.expect(load_config(probe_config('        on "scroll" "Quick" reach="all" mode="while"', quick)), "a while-mode ripple loads")
    settle_to("A")
    with Held() as pointer:
        x, y = center("B")
        pointer.send(f"move:{x}:{y}")
        time.sleep(0.3)
        feeder = Feeder(pointer, ("axis:0:15",), 0.2)
        feeder.start()
        try:
            seen = set()

            def rippling():
                seen.update(painted_titles(screenshot(), TITLES, YELLOW))
                return len(seen) == len(TITLES)

            checks.expect(wait_for(rippling, 30, 0.2), f"the ripple keeps repeating on every window while scrolling goes on ({sorted(seen)})")
        finally:
            feeder.stop()


def scrolling_plays_while(checks):
    while_mode_lasts_as_long_as_the_input(checks, ("axis:0:15",), "scroll", set(TITLES), "scrolling")


def typing_plays_while(checks):
    while_mode_lasts_as_long_as_the_input(checks, ((KEY_A, 1), (KEY_A, 0)), "keypress", set(TITLES), "typing")


def main():
    Checks().run(light_front_crosses_the_window_in_between, a_click_sends_a_front_over_every_window, while_mode_repeats_one_shot_layers,
                 scrolling_plays_while, typing_plays_while)


if __name__ == "__main__":
    main()
