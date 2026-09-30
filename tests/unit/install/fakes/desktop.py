import json
import os
import shutil
import sys
from pathlib import Path

import kconfig
import state


def kglobalaccel(method, values):
    store = state.shortcuts()
    count = int(values[0]) if values and values[0].isdigit() else 0
    name = state.action_name(*values[1:3]) if count else state.action_name(*values[:2])
    if method == "shortcut":
        keys = store.get(name, {}).get("keys", [])
        print(" ".join(["ai", str(len(keys)), *map(str, keys)]))
        return 0
    if method == "doRegister":
        store.setdefault(name, {"keys": [], "names": values[1:5]})
    elif method in ("setForeignShortcut", "setShortcut"):
        codes = [int(code) for code in values[6:6 + int(values[5])]]
        flags = int(values[6 + len(codes)]) if method == "setShortcut" else 2
        if name not in store:
            if method == "setForeignShortcut":
                return 0
            store[name] = {"keys": [], "names": values[1:5]}
        if flags & 2 or not store[name]["keys"]:
            state.assign(store, name, codes)
        if method == "setShortcut":
            keys = store[name]["keys"]
            print(" ".join(["ai", str(len(keys)), *map(str, keys)]))
    elif method == "unregister":
        print("b", "true" if store.pop(name, None) is not None else "false")
    state.save("kglobalaccel", store)
    return 0


def busctl(arguments):
    call = arguments[arguments.index("call") + 1:]
    service, method, values = call[0], call[3], call[5:]
    if service == "org.kde.kglobalaccel":
        return kglobalaccel(method, values)
    return 0


def kwin(method, values):
    kwin_state = state.load("kwin", {"running": True, "loaded": [], "refuse": []})
    if not kwin_state["running"]:
        print("Error: org.kde.KWin is not running", file=sys.stderr)
        return 1
    if method.endswith(".loadEffect"):
        if values[0] not in kwin_state["refuse"] and values[0] not in kwin_state["loaded"]:
            kwin_state["loaded"].append(values[0])
        print("(true,)")
    elif method.endswith(".unloadEffect"):
        kwin_state["loaded"] = [effect for effect in kwin_state["loaded"] if effect != values[0]]
        if values[0].startswith("konveyor_effect"):
            state.save("kglobalaccel", {name: entry for name, entry in state.shortcuts().items() if entry["names"][0] != "konveyor"})
    elif method.endswith(".isEffectLoaded"):
        print("(true,)" if values[0] in kwin_state["loaded"] else "(false,)")
    elif method.endswith("Properties.Get"):
        print("(<[" + ", ".join(f"'{effect}'" for effect in kwin_state["loaded"]) + "]>,)")
    state.save("kwin", kwin_state)
    return 0


def plasma(values):
    plasma_state = state.load("plasma", {"running": True, "scripts": []})
    if not plasma_state["running"]:
        print("Error: org.kde.plasmashell is not running", file=sys.stderr)
        return 1
    plasma_state["scripts"].append(values[0])
    state.save("plasma", plasma_state)
    print("('',)")
    return 0


def gdbus(arguments):
    destination = arguments[arguments.index("--dest") + 1]
    method = arguments[arguments.index("--method") + 1]
    values = arguments[arguments.index("--method") + 2:]
    if destination == "org.kde.KWin":
        return kwin(method, values)
    if destination == "org.kde.plasmashell":
        return plasma(values)
    return 0


def start_kontrol_panel():
    store = state.shortcuts()
    name = state.action_name("konveyor-kontrol-panel", "toggle")
    if name not in store:
        store[name] = {"keys": [], "names": ["konveyor-kontrol-panel", "toggle", "Kontrol Panel", "Toggle"]}
        state.assign(store, name, [state.key_code("Meta"), state.key_code("Alt+F1")])
    state.save("kglobalaccel", store)


def systemctl(arguments):
    systemd = state.load("systemd", {"environment": {}, "enabled": []})
    arguments = [argument for argument in arguments if argument not in ("--user", "--now")]
    command, values = arguments[0], arguments[1:]
    units = Path(os.environ["XDG_CONFIG_HOME"]) / "systemd" / "user"
    if command == "show-environment":
        print("\n".join(f"{key}={value}" for key, value in sorted(systemd["environment"].items())))
    elif command == "set-environment":
        systemd["environment"].update(value.split("=", 1) for value in values)
    elif command == "unset-environment":
        for value in values:
            systemd["environment"].pop(value, None)
    elif command == "enable":
        if not (units / values[0]).exists():
            print(f"Failed to enable unit: Unit file {values[0]} does not exist.", file=sys.stderr)
            return 1
        systemd["enabled"] = sorted(set(systemd["enabled"]) | {values[0]})
    elif command in ("restart", "start") and values == ["konveyor-kontrol-panel.service"]:
        start_kontrol_panel()
    elif command == "disable":
        if values[0] not in systemd["enabled"]:
            return 1
        systemd["enabled"].remove(values[0])
    state.save("systemd", systemd)
    return 0


def kpackagetool(arguments):
    plasmoids = Path(os.environ["XDG_DATA_HOME"]) / "plasma" / "plasmoids"
    mode, target = arguments[2], arguments[3]
    if mode == "-r":
        if not (plasmoids / target).is_dir():
            return 1
        shutil.rmtree(plasmoids / target)
        return 0
    plugin = json.loads((Path(target) / "metadata.json").read_text())["KPlugin"]["Id"]
    installed = (plasmoids / plugin).is_dir()
    if installed != (mode == "-u"):
        return 1
    (plasmoids / plugin).mkdir(parents=True, exist_ok=True)
    shutil.copy2(Path(target) / "metadata.json", plasmoids / plugin / "metadata.json")
    return 0


def konveyor(arguments):
    if arguments != ["restore-shortcuts"]:
        return 0
    path = Path(os.environ["XDG_STATE_HOME"]) / "konveyorstaterc"
    groups = kconfig.read(path)
    store = state.shortcuts()
    released = [entries for group, entries in groups.items() if group[:1] == ("ReleasedShortcuts",)]
    for entries in released:
        name = state.action_name(entries["Component"], entries["Action"])
        if name in store:
            codes = [state.key_code(key) for key in entries["Keys"].split(",") if key]
            state.assign(store, name, codes)
    state.save("kglobalaccel", store)
    kconfig.write(path, {group: entries for group, entries in groups.items() if group[:1] != ("ReleasedShortcuts",)})
    print(f"Restored {len(released)} KDE shortcuts")
    return 0


HANDLERS = {
    "busctl": busctl,
    "gdbus": gdbus,
    "systemctl": systemctl,
    "kwriteconfig6": kconfig.kwriteconfig,
    "kreadconfig6": kconfig.kreadconfig,
    "kpackagetool6": kpackagetool,
    "konveyor": konveyor,
}
