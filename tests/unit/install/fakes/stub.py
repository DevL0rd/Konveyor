#!/usr/bin/env -S python3 -S
import importlib
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)))

import state

MODULES = {
    "busctl": "desktop", "gdbus": "desktop", "systemctl": "desktop", "kwriteconfig6": "desktop", "kreadconfig6": "desktop",
    "kpackagetool6": "desktop", "konveyor": "desktop", "cmake": "system", "toolbox": "system", "distrobox": "system",
    "podman": "system", "rpm": "system", "pacman": "system", "dpkg": "system", "runuser": "system", "id": "system",
    "getent": "system", "date": "system",
}


def main():
    name = os.path.basename(sys.argv[0])
    arguments = sys.argv[1:]
    stdin = sys.stdin.read() if name == "sudo" and "tee" in arguments else None
    state.record([name, *arguments], stdin)
    if name == "sudo" and os.environ.get("STUB_INSIDE") and arguments[0] in MODULES:
        name, arguments = arguments[0], arguments[1:]
    if name not in MODULES:
        return 0
    return importlib.import_module(MODULES[name]).HANDLERS[name](arguments)


sys.exit(main())
