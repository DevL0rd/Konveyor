#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import activate, konveyor_action, qdbus, wait_for

PANEL = 'var p = panels()[0]; p.currentConfigGroup = ["Konveyor"]; print(p.lengthMode + "|" + p.readConfig("savedLengthMode", ""));'
WIDGETS = 'var d = desktops()[0]; d.currentConfigGroup = ["General"]; print(String(d.readConfig("hideDesktopWidgets", "unset")));'


def plasma(script):
    return subprocess.run(["qdbus6", "org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell.evaluateScript", script],
                          capture_output=True, text=True).stdout.strip()


def panel():
    return plasma(PANEL)


def widgets():
    return plasma(WIDGETS)


def shell_ready(checks):
    checks.expect(wait_for(lambda: plasma("print(panels().length);") == "1", 180, 1), f"plasmashell is up with one panel ({plasma('print(panels().length);')!r})")
    plasma('panels()[0].lengthMode = "fit";')
    checks.expect(wait_for(lambda: panel() == "fit|"), f"the panel fits its content ({panel()})")


def fill_panels(checks):
    activate("A")
    checks.equal(konveyor_action("maximize-window-to-edges"), "", "maximize A to the edges")
    checks.expect(wait_for(lambda: panel() == "fill|fit"), f"the panel fills the screen and remembers it was fitting ({panel()})")
    checks.equal(konveyor_action("maximize-window-to-edges"), "", "restore A")
    checks.expect(wait_for(lambda: panel() == "fit|"), f"the panel goes back ({panel()})")


def hide_widgets(checks):
    checks.expect(wait_for(lambda: widgets() == "true"), f"desktop widgets are hidden while windows are visible ({widgets()})")
    checks.equal(konveyor_action("focus-workspace", "2"), "", "switch to the empty workspace")
    checks.expect(wait_for(lambda: widgets() == "false"), f"desktop widgets come back on an empty workspace ({widgets()})")
    checks.equal(konveyor_action("focus-workspace", "1"), "", "switch back")
    checks.expect(wait_for(lambda: widgets() == "true"), "and hide again")


def unload_while_filled(checks):
    activate("A")
    checks.equal(konveyor_action("maximize-window-to-edges"), "", "maximize A again")
    checks.expect(wait_for(lambda: panel() == "fill|fit"), f"the panel fills ({panel()})")
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.unloadEffect", "konveyor_effect")
    checks.expect(wait_for(lambda: panel() == "fit|"), f"unloading Konveyor gives the panel its width back ({panel()})")
    checks.expect(wait_for(lambda: widgets() == "false"), f"and shows the desktop widgets ({widgets()})")


def main():
    Checks().run(shell_ready, fill_panels, hide_widgets, unload_while_filled)


if __name__ == "__main__":
    main()
