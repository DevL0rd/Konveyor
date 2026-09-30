import configparser
import os
from pathlib import Path

import dbus

SERVICE = "org.kde.kglobalaccel"


def kglobalaccel():
    return dbus.Interface(dbus.SessionBus().get_object(SERVICE, "/kglobalaccel"), "org.kde.KGlobalAccel")


def all_shortcuts():
    result = {}
    for path in kglobalaccel().allComponents():
        component = dbus.Interface(dbus.SessionBus().get_object(SERVICE, path), "org.kde.kglobalaccel.Component")
        for info in component.allShortcutInfos():
            result[(str(info[2]), str(info[0]))] = [int(key) for key in info[6] if int(key)]
    return result


def shortcut(component, action):
    return [int(key) for key in kglobalaccel().shortcut(dbus.Array([component, action, "", ""], signature="s")) if int(key)]


def set_foreign(component, action, keys):
    sequences = dbus.Array([dbus.Struct([dbus.Array([key], signature="i")], signature=None) for key in keys], signature="(ai)")
    kglobalaccel().setForeignShortcutKeys(dbus.Array([component, action, "", ""], signature="s"), sequences)


def released():
    parser = configparser.ConfigParser(interpolation=None, strict=False)
    parser.optionxform = str
    path = Path(os.environ["XDG_STATE_HOME"]) / "konveyorstaterc"
    if path.exists():
        parser.read(path)
    prefix = "ReleasedShortcuts]["
    return {section[len(prefix):]: dict(parser[section]) for section in parser.sections() if section.startswith(prefix)}
