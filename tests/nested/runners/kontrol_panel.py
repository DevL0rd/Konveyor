#!/usr/bin/env python3
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

import dbus
from checks import Checks
from fakepointer import binary, click, keys, move, tap
from keycodes import KEY_CODES, MODIFIER_CODES
from kwinsession import active_title, for_window, marked_lines, open_client, qdbus, run_script, wait_for, window_minimized
from nested import build_dir

PROBE = "notes.txt"
APP = "probe editor"
SERVICE = ("org.devl0rd.KontrolPanel", "/KontrolPanel")
META, ALT = MODIFIER_CODES["Super"], MODIFIER_CODES["Alt"]


def panel(method, *arguments):
    proxy = dbus.SessionBus().get_object(*SERVICE)
    return getattr(dbus.Interface(proxy, SERVICE[0]), method)(*arguments)


def is_open():
    return bool(panel("IsOpen"))


def allow_fake_input():
    desktop = Path(os.environ["XDG_DATA_HOME"]) / "applications" / "konveyor-fakepointer.desktop"
    desktop.write_text(f"[Desktop Entry]\nType=Application\nName=Fake input\nExec={binary()}\nNoDisplay=true\n"
                       "X-KDE-Wayland-Interfaces=org_kde_kwin_fake_input\n")
    subprocess.run(["kbuildsycoca6"], capture_output=True, check=True)
    return wait_for(lambda: subprocess.run([binary(), "move", "960", "540"], capture_output=True).returncode == 0, 30)


def press(code):
    keys((code, 1), (code, 0))


def type_text(text):
    for letter in text:
        press(KEY_CODES["space" if letter == " " else letter.upper()])


def outputs():
    printed = run_script('for (const o of workspace.screens) { const g = o.geometry; print("MARK|" + o.name + "|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height); }')
    return {name: tuple(float(v) for v in values) for name, *values in (line.split("|") for line in printed)}


def panel_windows():
    printed = run_script('for (const w of workspace.windowList()) { if (!w.deleted && w.resourceClass == "konveyor-kontrol-panel") { const g = w.frameGeometry; '
                         'print("MARK|" + (w.wantsInput ? "card" : "backdrop") + "|" + w.output.name + "|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height); } }')
    return {role: (name, tuple(float(v) for v in values)) for role, name, *values in (line.split("|") for line in printed)}


def card():
    return panel_windows().get("card")


def backdrop():
    return panel_windows().get("backdrop")


def first_card_geometry(action):
    marker = f"FIRST{time.monotonic_ns()}"
    body = ('workspace.windowAdded.connect(w => { if (w.resourceClass == "konveyor-kontrol-panel" && w.wantsInput) { const g = w.frameGeometry; '
            f'print("{marker}|" + w.output.name + "|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height); }} }});')
    with tempfile.NamedTemporaryFile("w", suffix=".js", delete=False) as script:
        script.write(body)
    script_id = qdbus("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting.loadScript", script.name, marker)
    qdbus("org.kde.KWin", f"/Scripting/Script{script_id}", "org.kde.kwin.Script.run")
    action()
    shown = wait_for(lambda: marked_lines(marker), 30)
    qdbus("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting.unloadScript", marker)
    os.unlink(script.name)
    if not shown:
        return None
    name, *values = shown[0].split("|")
    return name, tuple(float(v) for v in values)


def inside(rect, area):
    return area[0] <= rect[0] and area[1] <= rect[1] and rect[0] + rect[2] <= area[0] + area[2] and rect[1] + rect[3] <= area[1] + area[3]


def opened_and_focused():
    return wait_for(lambda: is_open() and active_title() == "", 30)


def closed():
    return wait_for(lambda: not is_open() and not panel_windows(), 30)


def search_switches_to_a_minimized_window(checks):
    open_client(PROBE)
    run_script(for_window(PROBE, "w.minimized = true;"))
    checks.expect(wait_for(lambda: window_minimized(PROBE), 30), f"{PROBE} is minimized")
    panel("Toggle")
    if not checks.expect(opened_and_focused(), "Toggle opens the Kontrol Panel with focus"):
        return
    type_text(APP)
    press(KEY_CODES["Return"])
    checks.expect(wait_for(lambda: active_title() == PROBE and not window_minimized(PROBE), 30), f"Enter on {APP} switched to {PROBE}")
    checks.expect(wait_for(lambda: not is_open(), 30), "switching windows closed the Kontrol Panel")


def meta_and_alt_f1_open_and_close_it(checks):
    press(META)
    checks.expect(opened_and_focused(), "Meta opens the Kontrol Panel with focus")
    press(META)
    checks.expect(closed(), "Meta again closes it")
    tap([ALT], KEY_CODES["F1"])
    checks.expect(opened_and_focused(), "Alt+F1 opens it")
    press(KEY_CODES["Escape"])
    checks.expect(closed(), "Escape closes it")


def clicking_beside_the_card_closes_it(checks):
    move(960, 540)
    press(META)
    checks.expect(opened_and_focused(), "Meta opens the Kontrol Panel")
    click(40, 540)
    checks.expect(closed(), "a click on the dimmed backdrop beside the card closes it")


def asking_for_a_page_while_open_switches_or_closes(checks):
    panel("Open", "apps")
    checks.expect(opened_and_focused(), "Open apps opens the Kontrol Panel")
    panel("Open", "games")
    checks.expect(is_open() and card() is not None, "Open games while it is open keeps it open")
    checks.expect(active_title() == "", "the Kontrol Panel keeps focus after switching pages")
    panel("Open", "games")
    checks.expect(closed(), "Open games again closes it")


def it_opens_on_the_output_under_the_pointer_and_fits_it(checks):
    result = subprocess.run(["kscreen-doctor", "output.Virtual-1.scale.2"], capture_output=True, text=True)
    checks.expect(result.returncode == 0 and "not found" not in result.stderr + result.stdout, f"output Virtual-1 is scaled to 2 {result.stderr}")
    checks.expect(wait_for(lambda: outputs().get("Virtual-1", (0, 0, 0, 0))[2] == 960, 30), "Virtual-1 is 960 logical pixels wide")
    for name, area in sorted(outputs().items(), reverse=True):
        move(area[0] + area[2] / 2, area[1] + area[3] / 2)
        first = first_card_geometry(lambda: press(META))
        checks.expect(opened_and_focused(), f"Meta opens the Kontrol Panel with the pointer on {name}")
        checks.expect(first is not None and first[0] == name and inside(first[1], area) and first[1][2] > area[2] * 0.6,
                      f"the card appears on {name} {area} already sized for it, it appeared at {first}")
        checks.equal(backdrop(), (name, area), f"the backdrop covers {name}")
        press(META)
        checks.expect(closed(), f"Meta closes it on {name}")


def main():
    checks = Checks()
    checks.expect(allow_fake_input(), "KWin lets the test send input")
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    process = subprocess.Popen([str(build_dir() / "bin" / "konveyor-kontrol-panel"), os.environ["KONVEYOR_KONTROL_PANEL_DIR"]],
                               stdout=log, stderr=subprocess.STDOUT)
    if checks.expect(wait_for(lambda: dbus.SessionBus().name_has_owner(SERVICE[0]), 60), "the Kontrol Panel takes its bus name"):
        checks.run(it_opens_on_the_output_under_the_pointer_and_fits_it, meta_and_alt_f1_open_and_close_it, clicking_beside_the_card_closes_it,
                   asking_for_a_page_while_open_switches_or_closes, search_switches_to_a_minimized_window)
    else:
        checks.run()
    process.terminate()
    process.wait(timeout=30)


if __name__ == "__main__":
    main()
