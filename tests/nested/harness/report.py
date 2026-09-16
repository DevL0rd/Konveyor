#!/usr/bin/env python3
import json

import dbus


def effect_loaded():
    kwin = dbus.Interface(dbus.SessionBus().get_object("org.kde.KWin", "/Effects"), "org.kde.kwin.Effects")
    return bool(kwin.isEffectLoaded("konveyor_effect"))


def query(method):
    konveyor = dbus.Interface(dbus.SessionBus().get_object("org.kde.Konveyor", "/Konveyor"), "org.kde.Konveyor")
    return json.loads(str(getattr(konveyor, method)()))


def main():
    report = {"effect_loaded": effect_loaded()}
    if report["effect_loaded"]:
        for name in ("Windows", "Workspaces", "Outputs", "Binds"):
            report[name.lower()] = query(name)
    print(json.dumps(report))


if __name__ == "__main__":
    main()
