#!/usr/bin/env python3
"""Audit Konveyor's key binds against KDE's global shortcut registry.

Every keyboard bind must own a registry entry with a real key sequence, and no
entry may share a sequence with another component. Pointer binds never reach
KDE: the effect's own input filter handles them.
"""
import json
import re
import subprocess
import sys
from pathlib import Path

SHORTCUTS_FILE = Path.home() / ".config/kglobalshortcutsrc"
COMPONENT = "konveyor"


def konveyor_binds():
    raw = subprocess.run(["konveyor", "msg", "--json", "binds"], capture_output=True, text=True, check=True).stdout
    return json.loads(raw)


def registry():
    entries = {}
    group = None
    for line in SHORTCUTS_FILE.read_text(encoding="utf-8").splitlines():
        header = re.fullmatch(r"\[(.+)\]", line)
        if header:
            group = header.group(1)
            continue
        if group is None or "=" not in line or line.startswith("_k_"):
            continue
        name, value = line.split("=", 1)
        fields = re.split(r"(?<!\\),", value, maxsplit=2)
        if len(fields) < 3:
            continue
        keys = [k for k in fields[0].split("\t") if k and k != "none"]
        entries[(group, name)] = keys
    return entries


def is_pointer(label):
    return "WheelScroll" in label or "Mouse" in label


def main():
    binds = [b for b in konveyor_binds() if b.get("key")]
    entries = registry()
    keyboard = [b for b in binds if not is_pointer(b["key"])]
    pointer = [b for b in binds if is_pointer(b["key"])]

    owners = {}
    for (group, name), keys in entries.items():
        for key in keys:
            owners.setdefault(key, []).append((group, name))

    problems = []
    for bind in keyboard:
        name = f"{COMPONENT}-{bind['key']}"
        keys = entries.get((COMPONENT, name))
        if keys is None:
            problems.append(f"NOT REGISTERED   {bind['key']:28} {bind['action']['name']}")
            continue
        if not keys:
            problems.append(f"NO KEY ASSIGNED  {bind['key']:28} {bind['action']['name']}")
            continue
        for key in keys:
            others = [o for o in owners.get(key, []) if o[0] != COMPONENT]
            if others:
                problems.append(f"SHADOWED         {bind['key']:28} {bind['action']['name']}  also: {others}")

    seen = {}
    for bind in binds:
        seen.setdefault(bind["key"], []).append(bind["action"]["name"])
    for key, actions in sorted(seen.items()):
        if len(actions) > 1:
            problems.append(f"DUPLICATE BIND   {key:28} {actions}")

    print(f"keyboard binds: {len(keyboard)}  registered: {len(keyboard) - len([p for p in problems if 'REGISTERED' in p])}")
    print(f"pointer binds:  {len(pointer)}  (handled by the effect, not KDE)")
    for problem in problems:
        print("  " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
