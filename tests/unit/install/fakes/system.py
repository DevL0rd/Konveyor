import json
import os
import sys
from pathlib import Path

import state

INSTALLED = [
    "{plugins}/kwin/effects/plugins/konveyor_effect.so",
    "{plugins}/kwin/effects/plugins/process_monitor_telemetry.so",
    "{plugins}/plasma/kcms/systemsettings/kcm_konveyor.so",
    "lib/qml/org/kde/konveyor/settings/qmldir",
    "bin/konveyor",
    "bin/konveyor-kontrol-panel",
    "share/konveyor/default-config.kdl",
]


def option(arguments, name):
    return arguments[arguments.index(name) + 1]


def cmake(arguments):
    if "-S" in arguments:
        build = Path(option(arguments, "-B"))
        build.mkdir(parents=True, exist_ok=True)
        defines = dict(argument[2:].split("=", 1) for argument in arguments if argument.startswith("-D"))
        (build / "stub-cache.json").write_text(json.dumps(defines))
        return 0
    if "--build" in arguments:
        plugins = Path(option(arguments, "--build")) / "bin" / "kwin" / "effects" / "plugins"
        plugins.mkdir(parents=True, exist_ok=True)
        for plugin in ("konveyor_effect", "process_monitor_telemetry"):
            (plugins / f"{plugin}.so").write_text(f"{plugin} {os.environ.get('STUB_BUILD', 'build')}\n")
        return 0
    build = Path(option(arguments, "--install"))
    defines = json.loads((build / "stub-cache.json").read_text())
    prefix = Path(defines["CMAKE_INSTALL_PREFIX"])
    plugins = "lib/qt6/plugins" if defines["KDE_INSTALL_USE_QT_SYS_PATHS"] == "ON" else "lib/plugins"
    files = json.loads(os.environ["STUB_INSTALLED"]) if "STUB_INSTALLED" in os.environ else INSTALLED
    manifest = []
    for relative in files:
        target = prefix / relative.format(plugins=plugins)
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.is_symlink() or target.exists():
            target.unlink()
        if target.parent.name == "bin":
            target.symlink_to(Path(os.environ["STUB_BIN"]) / target.name)
        else:
            target.write_text(f"{relative}\n")
        manifest.append(str(target))
    (build / "install_manifest.txt").write_text("\n".join(manifest))
    return 0


def run_inside(box, command):
    os.environ["STUB_INSIDE"] = box
    os.execvp(command[0], command)


def podman_state():
    return state.load("podman", {"containers": {}, "images": []})


def create(boxes, name, image):
    boxes["containers"][name] = image
    if image not in boxes["images"]:
        boxes["images"].append(image)


def toolbox(arguments):
    boxes = podman_state()
    if arguments[0] == "run":
        run_inside(option(arguments, "--container"), arguments[3:])
    if "create" in arguments:
        create(boxes, arguments[-1], "registry.fedoraproject.org/fedora-toolbox:" + option(arguments, "--release"))
    if arguments[0] == "rm":
        boxes["containers"].pop(arguments[-1], None)
    state.save("podman", boxes)
    return 0


def distrobox(arguments):
    boxes = podman_state()
    if arguments[0] == "enter":
        run_inside(arguments[1], arguments[arguments.index("--") + 1:])
    if arguments[0] == "create":
        create(boxes, option(arguments, "--name"), option(arguments, "--image"))
    state.save("podman", boxes)
    return 0


def podman(arguments):
    boxes = podman_state()
    if arguments[:2] == ["container", "exists"]:
        return 0 if arguments[2] in boxes["containers"] else 1
    if arguments[:2] == ["image", "exists"]:
        return 0 if arguments[2] in boxes["images"] else 1
    if arguments[:2] == ["image", "rm"]:
        boxes["images"].remove(arguments[2])
    elif arguments[0] == "ps" and "--filter" in arguments:
        image = option(arguments, "--filter").split("=", 1)[1]
        print("\n".join(name for name, used in boxes["containers"].items() if used == image))
    elif arguments[0] == "ps":
        print("\n".join(boxes["containers"]))
    elif arguments[0] == "rm":
        boxes["containers"].pop(arguments[-1], None)
    state.save("podman", boxes)
    return 0


def packages():
    return state.load("packages", {"system": {}, "box": {}, "owned": [], "groups": {}, "repositories": {}})


def installed():
    return packages()["box" if os.environ.get("STUB_INSIDE") else "system"]


def owned(path):
    return 0 if path in packages()["owned"] else 1


def rpm(arguments):
    if arguments[0] == "-qf":
        return owned(arguments[1])
    print("\n".join(f"{name} {version}" for name, version in sorted(installed().items())))
    return 0


def pacman(arguments):
    if arguments[0] == "-Qoq":
        return owned(arguments[1])
    if arguments[0] == "-Qgq":
        print("\n".join(packages()["groups"].get(arguments[1], [])))
    elif arguments[0] == "-Q":
        names = arguments[1:] or sorted(installed())
        print("\n".join(f"{name} {installed()[name]}" for name in names if name in installed()))
    elif arguments[0] == "-Si":
        repository = packages()["repositories"].get(arguments[1], "extra")
        print(f"Repository      : {repository}\nArchitecture    : x86_64")
    elif arguments[0] == "-U" and os.environ.get("STUB_INSIDE"):
        known = packages()
        for name, version in known["system"].items():
            if any(url.endswith(f"/{name}-{version}-x86_64.pkg.tar.zst") for url in arguments) and name in known["repositories"]:
                known["box"][name] = version
        state.save("packages", known)
    return 0


def dpkg(arguments):
    return owned(arguments[1])


def runuser(arguments):
    command = arguments[arguments.index("--") + 1:]
    if command[0] == "env":
        kept = {"PATH", "HOME", "XDG_RUNTIME_DIR", "DBUS_SESSION_BUS_ADDRESS"}
        command = ["env"] + [argument for argument in command[1:] if argument != "-i" and argument.split("=", 1)[0] not in kept]
    os.execvp(command[0], command)


def identity(arguments):
    names = [argument for argument in arguments if not argument.startswith("-")]
    if names and names != ["tester"]:
        print(f"id: '{names[0]}': no such user", file=sys.stderr)
        return 1
    if arguments[:1] == ["-u"]:
        print(os.environ["STUB_OWNER_UID"] if names else os.environ.get("STUB_UID", os.getuid()))
    elif arguments[:1] == ["-g"]:
        print(os.getgid())
    elif arguments[:1] == ["-un"]:
        print("tester")
    else:
        print("tester wheel")
    return 0


def getent(arguments):
    uid = os.environ["STUB_OWNER_UID"]
    print(f"{arguments[1]}:x:{uid}:{uid}::{os.environ['STUB_OWNER_HOME']}:/bin/bash")
    return 0


def date(arguments):
    print(os.environ.get("STUB_DATE", "1700000000"))
    return 0


HANDLERS = {
    "cmake": cmake,
    "toolbox": toolbox,
    "distrobox": distrobox,
    "podman": podman,
    "rpm": rpm,
    "pacman": pacman,
    "dpkg": dpkg,
    "runuser": runuser,
    "id": identity,
    "getent": getent,
    "date": date,
}
