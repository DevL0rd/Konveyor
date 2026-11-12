#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import qdbus, wait_for
from shortcuts import all_shortcuts, kglobalaccel, released, set_foreign, shortcut

META = 0x10000000
KEY_R = 0x52
PRESET_WIDTH = ("konveyor", "konveyor-Super+R")
REBOOT = ("ksmserver", "Reboot Without Confirmation")
DESKTOP_DOWN = ("kwin", "Switch One Desktop Down")


def effect_keys():
    return all_shortcuts().get(PRESET_WIDTH, [])


def reload_effect():
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.unloadEffect", "konveyor_effect")
    return qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.loadEffect", "konveyor_effect") == "true"


def user_key_survives_a_restart(checks):
    checks.expect(wait_for(lambda: effect_keys() == [META | KEY_R]), f"Konveyor holds Meta+R at first ({effect_keys()})")
    set_foreign(*PRESET_WIDTH, [])
    kglobalaccel().doRegister(["ksmserver", "Reboot Without Confirmation", "Session Management", "Reboot Without Confirmation"])
    set_foreign(*REBOOT, [META | KEY_R])
    checks.equal(shortcut(*REBOOT), [META | KEY_R], "the user gives Meta+R to another component's action")
    checks.expect(reload_effect(), "the effect loads again, as at the next login")
    checks.expect(wait_for(lambda: any(component == "konveyor" and keys for (component, _), keys in all_shortcuts().items()), 30), "Konveyor registered its shortcuts again")
    checks.equal(shortcut(*REBOOT), [META | KEY_R], "the user's shortcut keeps Meta+R after Konveyor loads")
    checks.equal(shortcut("org.kde.spectacle.desktop", "RecordRegion"), [META | KEY_R], "another default holder of Meta+R is left with it too")
    checks.equal(effect_keys(), [], "Konveyor does not take Meta+R back")
    checks.expect("/".join(REBOOT) not in released(), f"the user's shortcut is not recorded as taken over ({sorted(released())})")
    checks.expect("/".join(DESKTOP_DOWN) in released(), "KDE's own default keys are still taken over")


def main():
    Checks().run(user_key_survives_a_restart)


if __name__ == "__main__":
    main()
