#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, cli, config_path
from fakepointer import chord, move, tap
from keycodes import MODIFIER_CODES, evdev_codes
from kwinsession import konveyor, qdbus, wait_for
from shortcuts import all_shortcuts, released, set_foreign, shortcut

META = 0x10000000
CTRL = 0x04000000
ALT = 0x08000000
KEY_F1 = 0x01000030
KEY_M = 0x4D
KEY_DOWN = 0x01000015
KEY_UP = 0x01000013
DESKTOP_DOWN = ("kwin", "Switch One Desktop Down")
DESKTOP_UP = ("kwin", "Switch One Desktop Up")
EDIT_TILES = ("kwin", "Edit Tiles")
MAXIMIZE = ("kwin", "Window Maximize")
SMALL_BINDS = """
binds {
    Mod+F1 { spawn "true"; }
    Mod+Ctrl+Up { focus-workspace-up; }
    Mod+MouseMiddle { focus-workspace-down; }
}
"""


WHEEL = {"WheelScrollDown": "axis:0:15", "WheelScrollUp": "axis:0:-15", "WheelScrollRight": "axis:1:15", "WheelScrollLeft": "axis:1:-15"}
MIDDLE_BUTTON = 0x112


def is_pointer(bind):
    return "Wheel" in bind["key"] or "Mouse" in bind["key"]


def keyboard_binds():
    return [bind for bind in json.loads(konveyor("Binds")) if not is_pointer(bind)]


def fires(bind, press):
    before = last_bind()["count"]
    press()
    after = wait_for(lambda: last_bind() if last_bind()["count"] > before else None, 15)
    return bool(after) and after["key"] == bind["key"] and after["action"] == bind["action"]["name"]


def pointer_press(label):
    *modifiers, trigger = label.split("+")
    codes = [MODIFIER_CODES[modifier] for modifier in modifiers]
    if trigger == "MouseMiddle":
        return lambda: chord(codes, f"button:{MIDDLE_BUTTON}:1", f"button:{MIDDLE_BUTTON}:0")
    return lambda: chord(codes, WHEEL[trigger])


def konveyor_keys():
    return {action: keys for (component, action), keys in all_shortcuts().items() if component == "konveyor" and keys}


def last_bind():
    return json.loads(konveyor("LastBind"))


def registered(checks):
    binds = keyboard_binds()
    shortcuts = all_shortcuts()
    checks.expect(len(binds) > 50, f"{len(binds)} keyboard binds are configured")
    missing = [bind["key"] for bind in binds if not shortcuts.get(("konveyor", f"konveyor-{bind['key']}"))]
    checks.equal(missing, [], "keyboard binds without a registered KDE shortcut")
    owners = {}
    for (component, action), keys in shortcuts.items():
        for key in keys:
            owners.setdefault(key, []).append((component, action))
    shadowed = sorted({owner for key in (k for (c, _), ks in shortcuts.items() if c == "konveyor" for k in ks)
                       for owner in owners[key] if owner[0] != "konveyor"})
    checks.equal(shadowed, [], "other components holding a Konveyor key")
    labels = [bind["key"] for bind in json.loads(konveyor("Binds"))]
    checks.equal(sorted({label for label in labels if labels.count(label) > 1}), [], "duplicate binds")
    state = released()
    for entry in (DESKTOP_DOWN, DESKTOP_UP, EDIT_TILES):
        checks.expect("/".join(entry) in state, f"{entry} is recorded as taken over ({sorted(state)})")
        checks.equal(shortcut(*entry), [], f"{entry} no longer has a key")


def firing(checks):
    unmapped = []
    failed = []
    for bind in keyboard_binds():
        codes = evdev_codes(bind["key"])
        if codes is None:
            unmapped.append(bind["key"])
            continue
        if not fires(bind, lambda: tap(*codes)):
            failed.append((bind["key"], bind["action"]["name"], last_bind()))
    checks.equal(unmapped, [], "binds the test cannot type")
    checks.equal(failed, [], "binds that did not reach Konveyor when pressed")
    move(960, 540)
    pointer = [bind for bind in json.loads(konveyor("Binds")) if is_pointer(bind)]
    checks.expect(len(pointer) >= 12, f"{len(pointer)} wheel binds are configured")
    checks.equal([(bind["key"], last_bind()) for bind in pointer if not fires(bind, pointer_press(bind["key"]))], [], "wheel binds that did not fire")


def reload(checks):
    set_foreign(*MAXIMIZE, [CTRL | ALT | KEY_M])
    checks.equal(shortcut(*MAXIMIZE), [CTRL | ALT | KEY_M], "a user shortcut for another component")
    config_path().write_text(SMALL_BINDS)
    checks.expect(wait_for(lambda: sorted(konveyor_keys()) == ["konveyor-Super+Ctrl+Up", "konveyor-Super+F1"]),
                  f"only the new binds stay registered after a reload ({sorted(konveyor_keys())})")
    checks.equal(konveyor_keys().get("konveyor-Super+F1"), [META | KEY_F1], "the new bind's key")
    checks.equal(shortcut(*DESKTOP_DOWN), [META | CTRL | KEY_DOWN], "a KDE shortcut whose key is no longer bound is given back")
    checks.equal(shortcut(*DESKTOP_UP), [], "a KDE shortcut whose key is still bound stays taken over")
    state = released()
    checks.expect("/".join(DESKTOP_DOWN) not in state and "/".join(DESKTOP_UP) in state and "/".join(EDIT_TILES) in state,
                  f"konveyorstaterc lists what is still taken over ({sorted(state)})")
    checks.equal(shortcut(*MAXIMIZE), [CTRL | ALT | KEY_M], "the user's shortcut for another component survives the reload")
    middle = next(bind for bind in json.loads(konveyor("Binds")) if bind["key"] == "Super+MouseMiddle")
    checks.expect(fires(middle, pointer_press(middle["key"])), "a mouse button bind fires")
    set_foreign("konveyor", "konveyor-Super+F1", [CTRL | ALT | KEY_F1])
    config_path().write_text(SMALL_BINDS + "\n")
    checks.expect(wait_for(lambda: konveyor_keys().get("konveyor-Super+F1") == [META | KEY_F1]),
                  f"the config decides Konveyor's own keys after a reload ({konveyor_keys().get('konveyor-Super+F1')})")


def restore_while_running(checks):
    result = cli("restore-shortcuts")
    checks.expect(result.returncode != 0 and "running" in result.stderr, f"restore-shortcuts refuses while Konveyor runs ({result.returncode}, {result.stdout!r}, {result.stderr!r})")
    checks.expect("/".join(DESKTOP_UP) in released(), "the refused restore keeps konveyorstaterc")


def unload(checks):
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.unloadEffect", "konveyor_effect")
    checks.expect(wait_for(lambda: not konveyor_keys()), f"unloading removes Konveyor's shortcuts ({konveyor_keys()})")
    checks.equal(shortcut(*DESKTOP_UP), [META | CTRL | KEY_UP], "unloading gives back the KDE shortcut")
    checks.expect(shortcut(*EDIT_TILES) != [], "unloading gives back Edit Tiles")
    checks.equal(released(), {}, "unloading empties konveyorstaterc")
    result = cli("restore-shortcuts")
    checks.equal((result.returncode, result.stdout.strip()), (0, "Restored 0 KDE shortcuts"), "restore-shortcuts after unloading")
    checks.equal(shortcut(*MAXIMIZE), [CTRL | ALT | KEY_M], "the user's shortcut survives unloading")


def main():
    Checks().run(registered, firing, reload, restore_while_running, unload)


if __name__ == "__main__":
    main()
