#!/usr/bin/env python3
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from bindlog import fired, last_bind, missed
from checks import Checks, load_config
from fakepointer import Held, chord, move, tap
from keycodes import KEY_CODES, MODIFIER_CODES
from kwinsession import qdbus, wait_for

SUPER, SHIFT = MODIFIER_CODES["Super"], MODIFIER_CODES["Shift"]
RIGHT_ALT = 100
H, L = KEY_CODES["H"], KEY_CODES["L"]
LAYOUT_BINDS = """
binds {
    Mod+H { spawn "true"; }
    Mod+Shift+H { spawn "true"; }
    Mod+L repeat=false { spawn "true"; }
}
"""
LEVEL3_BINDS = """
input {
    mod-key "ISO_Level3_Shift"
}
binds {
    Mod+H { spawn "true"; }
    Mod+MouseMiddle { spawn "true"; }
    Mod+WheelScrollDown { spawn "true"; }
    Super+J { spawn "true"; }
}
"""


def held_for(codes, seconds):
    before = last_bind()["count"]
    with Held() as held:
        held.send(*[(code, 1) for code in codes])
        time.sleep(seconds)
        held.send(*[(code, 0) for code in reversed(codes)])
    time.sleep(0.5)
    return last_bind()["count"] - before


def set_layout(index):
    qdbus("org.kde.keyboard", "/Layouts", "org.kde.KeyboardLayouts.setLayout", str(index))
    return wait_for(lambda: qdbus("org.kde.keyboard", "/Layouts", "org.kde.KeyboardLayouts.getLayout") == str(index), 10)


def binds_on_a_layout_without_the_letter(checks):
    checks.expect(load_config(LAYOUT_BINDS), "a config with letter binds loads")
    checks.equal(fired(lambda: tap([SUPER], H)), "Super+H", "Meta+H runs its bind on the US layout")
    checks.expect(set_layout(1), "the Russian layout is active")
    checks.equal(fired(lambda: tap([SUPER], H)), "Super+H", "Meta+H runs its bind on the Russian layout by key position")
    checks.equal(fired(lambda: tap([SUPER, SHIFT], H)), "Super+Shift+H", "Meta+Shift+H runs its own bind there")
    checks.expect(held_for([SUPER, H], 1.5) > 2, "holding Meta+H repeats its bind on the Russian layout")
    checks.equal(held_for([SUPER, L], 1.5), 1, "holding Meta+L runs a repeat=false bind once")
    checks.expect(set_layout(0), "the US layout is active again")
    checks.expect(held_for([SUPER, L], 1.5) == 1, "holding Meta+L runs it once on the US layout too")


def level3_mod_key(checks):
    checks.expect(load_config(LEVEL3_BINDS), "a config with ISO_Level3_Shift as the Mod key loads")
    sentinel = lambda: tap([SUPER], KEY_CODES["J"])
    checks.equal(fired(sentinel), "Super+J", "an ordinary Meta bind still runs")
    checks.expect(missed(lambda: tap([], H), sentinel, "Super+J"), "typing H alone runs no bind")
    checks.equal(fired(lambda: tap([RIGHT_ALT], H)), "ISO_Level3_Shift+H", "AltGr+H runs the Mod+H bind")
    move(960, 540)
    checks.equal(fired(lambda: chord([RIGHT_ALT], "button:274:1", "button:274:0")), "ISO_Level3_Shift+MouseMiddle",
                 "AltGr with the middle button runs the Mod+MouseMiddle bind")
    checks.equal(fired(lambda: chord([RIGHT_ALT], "axis:0:15")), "ISO_Level3_Shift+WheelScrollDown",
                 "AltGr with the wheel runs the Mod+WheelScrollDown bind")
    checks.expect(missed(lambda: chord([], "button:274:1", "button:274:0"), sentinel, "Super+J"), "a plain middle click runs no bind")


def main():
    Checks().run(binds_on_a_layout_without_the_letter, level3_mod_key)


if __name__ == "__main__":
    main()
