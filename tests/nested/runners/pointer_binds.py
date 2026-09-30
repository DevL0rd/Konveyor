#!/usr/bin/env python3
import json
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

import virtualtouchpad
from checks import Checks, default_config, load_config
from fakepointer import Held, chord, move
from keycodes import MODIFIER_CODES
from kwinsession import konveyor, qdbus, run_script, wait_for

SUPER, CTRL, ALT = (MODIFIER_CODES[name] for name in ("Super", "Ctrl", "Alt"))
MODIFIERS = ("Super", "Ctrl", "Alt", "Shift")
COMBOS = [tuple(name for bit, name in enumerate(MODIFIERS) if mask & (1 << bit)) for mask in range(1, 16)]
BUTTONS = {"MouseLeft": 0x110, "MouseRight": 0x111, "MouseMiddle": 0x112, "MouseBack": 0x113, "MouseForward": 0x114}
DIRECTIONS = {"ScrollDown": (False, 15), "ScrollUp": (False, -15), "ScrollRight": (True, 15), "ScrollLeft": (True, -15)}
KWIN_AXIS_COMBOS = {("Super", "Ctrl"), ("Super", "Alt")}
SENTINEL = "Super+MouseLeft"
WHEEL_UP = "axis:0:-15"
WHEEL_DOWN = "axis:0:15"
WHEEL_RIGHT = "axis:1:15"


def last_bind():
    return json.loads(konveyor("LastBind"))


def fired(press):
    before = last_bind()["count"]
    press()
    after = wait_for(lambda: last_bind() if last_bind()["count"] > before else None, 5)
    return after["key"] if after else None


def active_effects():
    return qdbus("org.kde.KWin", "/Effects", "org.freedesktop.DBus.Properties.Get", "org.kde.kwin.Effects", "activeEffects").split()


def zoomed():
    return "zoom" in active_effects()


def current_desktop():
    return run_script('print("MARK|" + workspace.currentDesktop.id);')[0]


def set_zoom_modifiers(modifiers):
    subprocess.run(["kwriteconfig6", "--file", "kwinrc", "--group", "Effect-zoom", "--key", "PointerAxisGestureModifiers", modifiers], check=True)
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.reconfigureEffect", "zoom")


def kwin_wheel_shortcuts_come_first(checks):
    move(960, 540)
    checks.expect(not zoomed(), "the screen starts unzoomed")
    checks.equal(fired(lambda: chord([SUPER, CTRL], WHEEL_UP)), None, "Meta+Ctrl+wheel up runs no Konveyor bind")
    checks.expect(wait_for(zoomed, 10), f"Meta+Ctrl+wheel up zooms in ({active_effects()})")
    checks.equal(fired(lambda: chord([SUPER, CTRL], WHEEL_DOWN)), None, "Meta+Ctrl+wheel down runs no Konveyor bind")
    checks.expect(wait_for(lambda: not zoomed(), 10), f"Meta+Ctrl+wheel down zooms back out ({active_effects()})")
    desktop = current_desktop()
    checks.equal(fired(lambda: chord([SUPER, ALT], WHEEL_DOWN)), None, "Meta+Alt+wheel down runs no Konveyor bind")
    checks.expect(wait_for(lambda: current_desktop() != desktop, 10), "Meta+Alt+wheel down switches KDE desktops")
    checks.equal(fired(lambda: chord([SUPER, ALT], WHEEL_UP)), None, "Meta+Alt+wheel up runs no Konveyor bind")
    checks.expect(wait_for(lambda: current_desktop() == desktop, 10), "Meta+Alt+wheel up switches back")
    checks.equal(fired(lambda: chord([SUPER, CTRL, ALT], WHEEL_DOWN)), "Super+Ctrl+Alt+WheelScrollDown",
                 "the default bind moves columns between workspaces with Meta+Ctrl+Alt+wheel")
    checks.equal(fired(lambda: chord([SUPER, CTRL], WHEEL_RIGHT)), "Super+Ctrl+WheelScrollRight",
                 "Meta+Ctrl with the horizontal wheel is still Konveyor's")
    checks.expect(not zoomed(), "Konveyor's horizontal Meta+Ctrl bind does not zoom")


def user_binds_on_kwin_wheel_shortcuts(checks):
    config = default_config().replace("Mod+Ctrl+Alt+WheelScrollUp", "Mod+Ctrl+WheelScrollUp")
    checks.expect(load_config(config), "a config binding Mod+Ctrl+WheelScrollUp loads")
    checks.equal(fired(lambda: chord([SUPER, CTRL], WHEEL_UP)), None, "KWin's zoom still takes Meta+Ctrl+wheel up from a user bind")
    checks.expect(wait_for(zoomed, 10), "the wheel zooms in")
    chord([SUPER, CTRL], WHEEL_DOWN)
    checks.expect(wait_for(lambda: not zoomed(), 10), "and zooms back out")
    set_zoom_modifiers("")
    checks.equal(fired(lambda: chord([SUPER, CTRL], WHEEL_UP)), "Super+Ctrl+WheelScrollUp",
                 "once zoom gives up its wheel modifiers the user's bind runs")
    checks.expect(not zoomed(), "and the screen stays unzoomed")
    set_zoom_modifiers("Meta+Control")
    checks.equal(fired(lambda: chord([SUPER, CTRL], WHEEL_UP)), None, "zoom takes the wheel back when its modifiers return")
    chord([SUPER, CTRL], WHEEL_DOWN)
    checks.expect(wait_for(lambda: not zoomed(), 10), "the screen is unzoomed again")
    load_config(default_config())


def triggers():
    for combo in COMBOS:
        for button in BUTTONS:
            yield combo, button
        for direction in DIRECTIONS:
            yield combo, "Wheel" + direction
            yield combo, "Touchpad" + direction


def label(combo, trigger):
    return "+".join((*combo, trigger))


def press(combo, trigger):
    codes = [MODIFIER_CODES[name] for name in combo]
    if trigger in BUTTONS:
        return lambda: chord(codes, f"button:{BUTTONS[trigger]}:1", f"button:{BUTTONS[trigger]}:0")
    horizontal, delta = DIRECTIONS[trigger.removeprefix("Wheel").removeprefix("Touchpad")]
    if trigger.startswith("Wheel"):
        return lambda: chord(codes, f"axis:{int(horizontal)}:{delta}")

    def scroll():
        with Held() as held:
            held.send(*[(code, 1) for code in codes])
            virtualtouchpad.scroll(horizontal, delta)
            held.send(*[(code, 0) for code in reversed(codes)])
    return scroll


def claimed_by_kwin(combo, trigger):
    return combo in KWIN_AXIS_COMBOS and trigger.endswith(("ScrollUp", "ScrollDown"))


def missed(action):
    before = last_bind()["count"]
    action()
    press(("Super",), "MouseLeft")()
    after = wait_for(lambda: last_bind() if last_bind()["count"] > before and last_bind()["key"] == SENTINEL else None, 10)
    return after is not None and after["count"] == before + 1


def every_pointer_bind(checks):
    body = "\n".join(f"    {label(combo, trigger)} {{ spawn \"true\"; }}" for combo, trigger in triggers())
    checks.expect(load_config("binds {\n" + body + "\n}\n"), f"a config with all {len(list(triggers()))} pointer binds loads")
    virtualtouchpad.add()
    move(960, 540)
    wrong = []
    for combo, trigger in triggers():
        if claimed_by_kwin(combo, trigger):
            if not missed(press(combo, trigger)):
                wrong.append((label(combo, trigger), "ran although KWin owns it"))
        elif fired(press(combo, trigger)) != label(combo, trigger):
            wrong.append((label(combo, trigger), last_bind()["key"]))
    checks.equal(wrong, [], "pointer binds that did not go to the right place")
    for name, action in (("an unmodified click", lambda: chord([], "button:272:1", "button:272:0")),
                         ("an unmodified wheel", lambda: chord([], "axis:0:15")),
                         ("an unmodified finger scroll", lambda: virtualtouchpad.scroll(False, 15)),
                         ("Meta with a mouse button no bind knows", lambda: chord([SUPER], "button:277:1", "button:277:0")),
                         ("a wheel event without movement", lambda: chord([SUPER], "axis:0:0"))):
        checks.expect(missed(action), f"{name} runs no bind")
    virtualtouchpad.remove()
    load_config(default_config())


def unmodified_pointer_binds(checks):
    checks.expect(load_config("binds {\n    MouseForward { spawn \"true\"; }\n    WheelScrollDown { spawn \"true\"; }\n"
                              "    TouchpadScrollLeft { spawn \"true\"; }\n    Super+MouseLeft { spawn \"true\"; }\n}\n"),
                  "a config with pointer binds that have no modifier loads")
    virtualtouchpad.add()
    move(960, 540)
    checks.equal(fired(press((), "MouseForward")), "MouseForward", "the forward button alone runs its bind")
    checks.equal(fired(press((), "WheelScrollDown")), "WheelScrollDown", "the wheel alone runs its bind")
    checks.equal(fired(press((), "TouchpadScrollLeft")), "TouchpadScrollLeft", "a finger scroll alone runs its bind")
    checks.expect(missed(press((), "MouseBack")), "the back button with no bind runs none")
    checks.expect(missed(press((), "WheelScrollUp")), "scrolling the other way runs no bind")
    checks.expect(missed(press(("Shift",), "MouseForward")), "Shift with the forward button runs no bind")
    virtualtouchpad.remove()
    load_config(default_config())


def main():
    Checks().run(kwin_wheel_shortcuts_come_first, user_binds_on_kwin_wheel_shortcuts, every_pointer_bind, unmodified_pointer_binds)


if __name__ == "__main__":
    main()
