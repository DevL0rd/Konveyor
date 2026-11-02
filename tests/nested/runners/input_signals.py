#!/usr/bin/env python3
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from fakepointer import Held
from kwinsession import activate, for_window, frame, managed_titles, qdbus, run_script, wait_for

STYLE = """
border-style "Probe" {
    layer "gradient" {
        colors "#ff0000" "#00ff00"
        drive "width" "scroll.speed"
        drive "opacity" "scroll.distance"
        drive "speed" "scroll.direction"
        drive "angle" "pointer.speed"
        drive "tail" "typing.rate"
    }
}
"""
EVENTS = """
    event-effects {
        on "click" "Probe" mode="once"
        on "scroll" "Probe" mode="once"
        on "keypress" "Probe" mode="once"
    }
"""
KEY_A = 30
LEFT_BUTTON = 0x110
CLIENTS = Path(__file__).resolve().parent.parent / "clients"
LOGS = {}
INPUT = Held()


def probing_config():
    config = default_config().replace('active-style "Default-Style"', 'active-style "Probe"', 1).replace('inactive-style "Default-Style-Inactive"', 'inactive-style "Probe"', 1)
    config = re.sub(r"focus-effects \{[^}]*\}", "", config)
    config = re.sub(r"event-effects \{[^}]*\}", "", config)
    return re.sub(r"^layout \{", "layout {" + EVENTS, config, count=1, flags=re.M) + STYLE


def quiet_config():
    return re.sub(r"^\s*(active|inactive|urgent)-style .*\n", "", default_config(), flags=re.M)


def signals():
    return json.loads(qdbus("org.kde.Konveyor", "/Konveyor", "org.kde.Konveyor.SignalFrames"))


def observing():
    return signals()["observing"]


def of(title):
    return next(window for window in signals()["windows"] if window["title"] == title)


def value(title, name):
    return of(title)["values"][name]


def fired(title, name):
    return of(title)["fired"][name]


def center(title):
    x, y, width, height = frame(title)
    return x + width / 2, y + height / 2


def open_logged_client(title, client):
    log = Path(os.environ["KONVEYOR_TEST_ROOT"]) / f"client-{title}.log"
    with open(log, "w") as out:
        process = subprocess.Popen(["qml6", str(CLIENTS / client), "--", title, "700", "500"], stdout=out, stderr=subprocess.STDOUT,
                                   env={**os.environ, "WAYLAND_DEBUG": "client"})
    if not wait_for(lambda: title in managed_titles(), 60):
        raise RuntimeError(f"the {title} window never appeared")
    LOGS[title] = log
    return process


def received(title, pattern):
    return len(re.findall(pattern, LOGS[title].read_text(errors="replace")))


def buttons(title):
    return received(title, rf"wl_pointer#\d+\.button\(\d+, \d+, {LEFT_BUTTON}, 1\)")


def wheels(title):
    return received(title, r"wl_pointer#\d+\.axis\(\d+, 0, ")


def key_presses(title):
    return received(title, rf"wl_keyboard#\d+\.key\(\d+, \d+, {KEY_A}, 1\)")


def point_at(x, y):
    INPUT.send(f"move:{x}:{y}")
    time.sleep(0.2)


def click_at(x, y):
    point_at(x, y)
    INPUT.send(f"button:{LEFT_BUTTON}:1", f"button:{LEFT_BUTTON}:0")
    time.sleep(0.5)


def press_a():
    INPUT.send((KEY_A, 1), (KEY_A, 0))


def rises(title, name, above=0.05):
    return wait_for(lambda: value(title, name) > above, 2, 0.01)


def nothing_observed_without_a_reference(checks):
    checks.expect(load_config(quiet_config()), "a config with no input driven layer loads")
    checks.expect(wait_for(lambda: not observing(), 5), "no spy is installed while nothing references an input signal")


def spy_follows_references(checks):
    checks.expect(load_config(probing_config()), "a config referencing input signals loads")
    checks.expect(wait_for(observing, 5), "the spy is installed once a style references an input signal")
    checks.expect(load_config(quiet_config()), "a config with no input driven layer loads again")
    checks.expect(wait_for(lambda: not observing(), 5), "the spy goes away with the last reference")
    checks.expect(load_config(probing_config()), "the probing config loads again")
    checks.expect(wait_for(observing, 5), "and it comes back")


def clicks_reach_the_window_under_the_pointer(checks):
    before = buttons("B")
    a, b = fired("A", "click"), fired("B", "click")
    x, y = center("B")
    for _ in range(3):
        click_at(x, y)
    checks.equal(fired("B", "click"), b + 3, "three clicks on B fire three click impulses on B")
    checks.equal(fired("A", "click"), a, "A's click count stays put")
    checks.equal((of("B")["x"], of("B")["y"]), (x, y), "the event point is where the pointer clicked")
    checks.expect(wait_for(lambda: buttons("B") == before + 3, 5), f"B got all three clicks ({buttons('B') - before})")


def wheel_moves_speed_direction_and_distance(checks):
    point_at(*center("B"))
    before = wheels("B")
    step = fired("B", "scroll.step")
    other = fired("A", "scroll.step")
    for _ in range(6):
        INPUT.send("axis:0:15")
    checks.expect(rises("B", "scroll.speed"), "wheel down raises scroll.speed on B")
    window = of("B")
    print(f"  down: {window['values']['scroll.speed']} {window['values']['scroll.distance']} {window['values']['scroll.direction']}")
    checks.expect(window["values"]["scroll.direction"] > 0.5, "wheel down reads a positive direction")
    checks.expect(window["values"]["scroll.distance"] > 0.1, "wheel down raises scroll.distance")
    checks.equal(window["directionY"], 1, "the event direction points down")
    checks.equal(fired("B", "scroll.step"), step + 6, "six notches fire six steps")
    checks.equal(fired("A", "scroll.step"), other, "A's step count stays put")
    checks.equal(value("A", "scroll.speed"), 0, "A's scroll.speed stays zero")
    checks.equal(value("A", "scroll.distance"), 0, "A's scroll.distance stays zero")
    time.sleep(1.0)
    for _ in range(6):
        INPUT.send("axis:0:-15")
    checks.expect(wait_for(lambda: value("B", "scroll.direction") < -0.5, 2, 0.01), "wheel up reads a negative direction")
    window = of("B")
    print(f"  up: {window['values']['scroll.speed']} {window['values']['scroll.direction']}")
    checks.equal(window["directionY"], -1, "the event direction points up")
    checks.expect(wait_for(lambda: wheels("B") == before + 12, 5), f"B got all twelve wheel events ({wheels('B') - before})")
    checks.expect(wait_for(lambda: value("B", "scroll.speed") == 0 and value("B", "scroll.direction") == 0, 20), "speed and direction decay to zero")
    checks.expect(wait_for(lambda: value("B", "scroll.distance") == 0, 30), "distance decays to zero")


def high_resolution_scroll_adds_up(checks):
    point_at(*center("B"))
    before = received("B", r"wl_pointer#\d+\.axis\(\d+, 0, 1\.")
    step = fired("B", "scroll.step")
    INPUT.send(*["axis:0:1"] * 30)
    checks.equal(fired("B", "scroll.step"), step + 2, "thirty high resolution deltas of one are two notches")
    checks.expect(rises("B", "scroll.speed"), "they raise scroll.speed")
    checks.expect(wait_for(lambda: received("B", r"wl_pointer#\d+\.axis\(\d+, 0, 1\.") == before + 30, 5), "B got all thirty")
    checks.expect(wait_for(lambda: value("B", "scroll.distance") == 0, 30), "and it decays")


def pointer_speed_follows_motion(checks):
    x, y = center("B")
    INPUT.glide((x - 150, y), (x + 150, y), steps=30)
    checks.expect(rises("B", "pointer.speed", 0.02), f"moving over B raises pointer.speed on B ({value('B', 'pointer.speed')})")
    checks.equal(value("A", "pointer.speed"), 0, "A's pointer.speed stays zero")
    checks.expect(wait_for(lambda: value("B", "pointer.speed") == 0, 10), "pointer.speed decays to zero")


def keys_reach_the_focused_window(checks):
    x, y, width, height = frame("A")
    click_at(x + width / 2, y + height - 40)
    activate("A")
    before = key_presses("A")
    press, other = fired("A", "keypress"), fired("B", "keypress")
    for _ in range(4):
        press_a()
    checks.expect(rises("A", "typing.rate"), "typing.rate rises on A")
    print(f"  typing: {value('A', 'typing.rate')}")
    checks.equal(fired("A", "keypress"), press + 4, "four keys fire four keypress impulses on the focused window")
    checks.equal(fired("B", "keypress"), other, "B's keypress count stays put")
    checks.equal(value("B", "typing.rate"), 0, "B's typing.rate stays zero")
    checks.expect(wait_for(lambda: key_presses("A") == before + 4, 5), f"A got all four keys ({key_presses('A') - before})")
    checks.expect(wait_for(lambda: value("A", "typing.rate") == 0, 20), "typing.rate decays to zero")


def impulses_decay(checks):
    click_at(*center("B"))
    checks.expect(wait_for(lambda: value("B", "click") < 0.01, 10), f"the click impulse decays ({value('B', 'click')})")


def fullscreen_stops_observing(checks):
    activate("A")
    run_script(for_window("A", "w.fullScreen = true;"))
    checks.expect(wait_for(lambda: not observing(), 10), "a fullscreen active window stops observing")
    press = fired("A", "keypress")
    before = key_presses("A")
    press_a()
    checks.equal(fired("A", "keypress"), press, "keys are not counted while fullscreen")
    checks.expect(wait_for(lambda: key_presses("A") == before + 1, 5), "A still gets the key while nothing observes")
    run_script(for_window("A", "w.fullScreen = false;"))
    checks.expect(wait_for(observing, 10), "observing comes back after fullscreen")


def reduce_motion_stops_observing(checks):
    config = probing_config()
    held = re.sub(r"^animations \{", "animations {\n    reduce-motion", config, count=1, flags=re.M)
    checks.expect(held != config, "the config has an animations block")
    checks.expect(load_config(held), "a reduce motion config loads")
    checks.expect(wait_for(lambda: not observing(), 5), "reduce motion stops observing")
    checks.expect(load_config(config), "the probing config loads again")
    checks.expect(wait_for(observing, 5), "observing comes back")


def main():
    checks = Checks()
    with INPUT:
        processes = [open_logged_client("A", "text-client.qml"), open_logged_client("B", "client.qml")]
        try:
            checks.run(nothing_observed_without_a_reference, spy_follows_references, clicks_reach_the_window_under_the_pointer,
                       wheel_moves_speed_direction_and_distance, high_resolution_scroll_adds_up, pointer_speed_follows_motion,
                       keys_reach_the_focused_window, impulses_decay, fullscreen_stops_observing, reduce_motion_stops_observing)
        finally:
            for process in processes:
                process.terminate()


if __name__ == "__main__":
    main()
