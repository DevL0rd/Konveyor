#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

import dbus
from fakepointer import binary, keys
from keycodes import KEY_CODES
from kwinsession import active_title, for_window, open_client, run_script, wait_for, window_minimized
from nested import build_dir

PROBE = "notes.txt"
APP = "probe editor"
SERVICE = ("org.devl0rd.KontrolPanel", "/KontrolPanel")


def panel(method):
    proxy = dbus.SessionBus().get_object(*SERVICE)
    return getattr(dbus.Interface(proxy, SERVICE[0]), method)()


def allow_fake_input():
    desktop = Path(os.environ["XDG_DATA_HOME"]) / "applications" / "konveyor-fakepointer.desktop"
    desktop.write_text(f"[Desktop Entry]\nType=Application\nName=Fake input\nExec={binary()}\nNoDisplay=true\n"
                       "X-KDE-Wayland-Interfaces=org_kde_kwin_fake_input\n")
    subprocess.run(["kbuildsycoca6"], capture_output=True, check=True)


def type_text(text):
    events = []
    for letter in text:
        code = KEY_CODES["space" if letter == " " else letter.upper()]
        events += [(code, 1), (code, 0)]
    keys(*events)


def switch_with_enter():
    for _ in range(5):
        keys((KEY_CODES["Return"], 1), (KEY_CODES["Return"], 0))
        if wait_for(lambda: active_title() == PROBE and not window_minimized(PROBE), 5):
            return True
    return False


def main():
    problems = []
    allow_fake_input()
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    open_client(PROBE)
    run_script(for_window(PROBE, "w.minimized = true;"))
    if not wait_for(lambda: window_minimized(PROBE), 30):
        problems.append(f"{PROBE} did not minimize")
    process = subprocess.Popen([str(build_dir() / "bin" / "konveyor-kontrol-panel"), os.environ["KONVEYOR_KONTROL_PANEL_DIR"]],
                               stdout=log, stderr=subprocess.STDOUT)
    if not wait_for(lambda: dbus.SessionBus().name_has_owner(SERVICE[0]), 60):
        problems.append("the Kontrol Panel never took its bus name")
    else:
        panel("Toggle")
        if not wait_for(lambda: bool(panel("IsOpen")) and active_title() == "", 30):
            problems.append(f"the Kontrol Panel did not take focus, {active_title()} has it")
        else:
            type_text(APP)
            if not switch_with_enter():
                problems.append(f"Enter on the open window result left {active_title()} active")
            elif bool(panel("IsOpen")):
                problems.append("the Kontrol Panel stayed open after switching windows")
            else:
                print(f"searching for {APP} switched to its minimized window {PROBE}")
    process.terminate()
    process.wait(timeout=30)
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
