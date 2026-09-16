#!/usr/bin/env python3
"""Press every Konveyor keyboard bind for real and confirm the effect ran it.

ydotool injects evdev events, so the whole chain is exercised: evdev -> KWin ->
KGlobalAccel -> Konveyor. The effect reports the action it last ran and how many
binds it has run, so a bind that is a no-op in the current layout still counts
as fired. What each action then does to the layout is covered by the unit tests.
"""
import json
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from keycodes import evdev_codes

os.environ.setdefault("YDOTOOL_SOCKET", f"/run/user/{os.getuid()}/.ydotool_socket")
SKIP = {"toggle-keyboard-shortcuts-inhibit", "quit", "power-off-monitors"}


def msg(*args):
    raw = subprocess.run(["konveyor", "msg", "--json", *args], capture_output=True, text=True, check=True).stdout
    return json.loads(raw)


def last_bind():
    raw = subprocess.run(
        ["qdbus6", "org.kde.Konveyor", "/Konveyor", "org.kde.Konveyor.LastBind"], capture_output=True, text=True, check=True
    ).stdout
    return json.loads(raw)


def press(label):
    codes = evdev_codes(label)
    if codes is None:
        return False
    modifiers, key = codes
    for code in modifiers:
        subprocess.run(["ydotool", "key", f"{code}:1"], capture_output=True, check=False)
    time.sleep(0.1)
    subprocess.run(["ydotool", "key", f"{key}:1", f"{key}:0"], capture_output=True, check=False)
    time.sleep(0.1)
    for code in reversed(modifiers):
        subprocess.run(["ydotool", "key", f"{code}:0"], capture_output=True, check=False)
    time.sleep(0.45)
    return True


def main():
    if subprocess.run(["pgrep", "-x", "ydotoold"], capture_output=True).returncode != 0:
        print("ydotoold is not running; start it with: sudo systemctl start ydotoold")
        return 2

    binds = [b for b in msg("binds") if b.get("key")]
    keyboard = [b for b in binds if "WheelScroll" not in b["key"] and "Mouse" not in b["key"]]
    tested = [b for b in keyboard if b["action"]["name"] not in SKIP]

    failures = []
    unmapped = []
    for bind in tested:
        before = last_bind()
        if not press(bind["key"]):
            unmapped.append(bind["key"])
            continue
        after = last_bind()
        if after["count"] == before["count"]:
            failures.append((bind["key"], bind["action"]["name"], "did not reach the effect"))
        elif after["key"] != bind["key"]:
            failures.append((bind["key"], bind["action"]["name"], f"ran {after['key']} instead"))

    print(f"keyboard binds: {len(keyboard)}   tested: {len(tested)}   skipped: {len(keyboard) - len(tested)}")
    print(f"fired: {len(tested) - len(failures) - len(unmapped)}")
    for key in unmapped:
        print(f"  NO KEYCODE IN TEST  {key}")
    for key, action, why in failures:
        print(f"  FAILED              {key:28} {action:34} {why}")
    print("RESULT:", "FAIL" if failures or unmapped else "PASS")
    return 1 if failures or unmapped else 0


if __name__ == "__main__":
    sys.exit(main())
