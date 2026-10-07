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
from kontrol_panel import SERVICE, card, panel, panel_windows
from kwinsession import CLIENTS, run_script, wait_for
from nested import build_dir
from screenshot import capture_workspace, close

GREEN = (0, 255, 0)
DIMMED_GREEN = (0, 115, 0)
WHITE = (255, 255, 255)
DIMMED_WHITE = (115, 115, 115)
CORNER = (4, 4)
TASKBAR = (960, 1050)
SCALE_SECONDS = 8


def shot():
    return capture_workspace(tempfile.mktemp(suffix=".png", dir=os.environ["KONVEYOR_TEST_ROOT"]))


def stacking():
    return run_script('for (const w of workspace.stackingOrder) { if (!w.deleted && (w.dock || w.resourceClass == "konveyor-kontrol-panel")) { '
                      'print("MARK|" + (w.dock ? "taskbar" : w.wantsInput ? "card" : "backdrop")); } }')


def backdrop_type():
    printed = run_script('for (const w of workspace.windowList()) { if (!w.deleted && w.resourceClass == "konveyor-kontrol-panel" && !w.wantsInput) { '
                         'print("MARK|" + (w.normalWindow ? "normal" : w.utility ? "utility" : "other")); } }')
    return printed[0] if printed else None


def card_left_edge(image, geometry):
    x, y, width, height = (int(value) for value in geometry)
    row = y + height // 2
    return next((column for column in range(max(x, 0), x + width // 2) if not close(image.getpixel((column, row)), DIMMED_GREEN, 6)), None)


def start_panel(settings):
    rc = Path(os.environ["XDG_CONFIG_HOME"]) / "konveyor" / "kontrolpanelrc"
    rc.parent.mkdir(exist_ok=True)
    rc.write_text("[General]\n" + "".join(f"{key}={value}\n" for key, value in settings.items()))
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    process = subprocess.Popen([str(build_dir() / "bin" / "konveyor-kontrol-panel"), os.environ["KONVEYOR_KONTROL_PANEL_DIR"]],
                               stdout=log, stderr=subprocess.STDOUT)
    if not wait_for(lambda: dbus.SessionBus().name_has_owner(SERVICE[0]), 60):
        raise RuntimeError("the Kontrol Panel never took its bus name")
    return process


def stop_panel(process):
    process.terminate()
    process.wait(timeout=30)
    wait_for(lambda: not dbus.SessionBus().name_has_owner(SERVICE[0]), 30)


def open_panel(checks, settings):
    process = start_panel(settings)
    panel("Toggle")
    checks.expect(wait_for(lambda: card() is not None, 30), f"{settings}: the card maps")
    time.sleep(1)
    early = shot()
    geometry = card()[1]
    kind = backdrop_type()
    time.sleep(SCALE_SECONDS)
    settled = shot()
    order = stacking()
    panel("Hide")
    checks.expect(wait_for(lambda: not panel_windows(), 30), f"{settings}: Hide closes it")
    stop_panel(process)
    time.sleep(SCALE_SECONDS)
    return early, settled, order, geometry, kind


def taskbar_ready(checks):
    subprocess.Popen(["qml6", str(CLIENTS / "backdrop-client.qml")])
    checks.expect(wait_for(lambda: "taskbar" in stacking(), 30), "the stand-in taskbar mapped")
    checks.expect(wait_for(lambda: close(shot().getpixel(TASKBAR), WHITE) and close(shot().getpixel(CORNER), GREEN), 30), "the wallpaper and taskbar are drawn")


def by_default_the_backdrop_scales_in_below_the_taskbar(checks):
    early, settled, order, _, kind = open_panel(checks, {})
    checks.equal(kind, "normal", "the backdrop is a normal window that window open effects animate")
    checks.expect(close(early.getpixel(CORNER), GREEN), f"the backdrop is still scaling in and leaves the corner bare {early.getpixel(CORNER)}")
    checks.expect(close(settled.getpixel(CORNER), DIMMED_GREEN), f"once in, the backdrop dims the whole screen {settled.getpixel(CORNER)}")
    checks.expect(close(settled.getpixel(TASKBAR), WHITE), f"the taskbar stays bright above the backdrop {settled.getpixel(TASKBAR)}")
    checks.expect(order.index("backdrop") < order.index("taskbar") < order.index("card"), f"the backdrop is stacked below the taskbar {order}")


def the_backdrop_can_cover_the_taskbar(checks):
    early, settled, order, _, kind = open_panel(checks, {"useTaskbarWhileOpen": "false"})
    checks.equal(kind, "normal", "covering the taskbar keeps the scale animation")
    checks.expect(close(early.getpixel(CORNER), GREEN), f"the backdrop still scales in {early.getpixel(CORNER)}")
    checks.expect(close(settled.getpixel(TASKBAR), DIMMED_WHITE), f"the backdrop dims the taskbar {settled.getpixel(TASKBAR)}")
    checks.expect(order.index("taskbar") < order.index("backdrop") < order.index("card"), f"the backdrop is stacked above the taskbar {order}")


def the_backdrop_can_fade_in_while_the_card_still_scales(checks):
    early, settled, order, geometry, kind = open_panel(checks, {"backdropAnimation": "fade"})
    checks.equal(kind, "utility", "the fading backdrop is a utility window that window open effects leave alone")
    checks.expect(close(early.getpixel(CORNER), DIMMED_GREEN), f"the backdrop dims the corner without scaling {early.getpixel(CORNER)}")
    checks.expect(close(settled.getpixel(TASKBAR), WHITE), f"the taskbar stays above a fading backdrop {settled.getpixel(TASKBAR)}")
    checks.expect(order.index("backdrop") < order.index("taskbar"), f"the fading backdrop is still stacked below the taskbar {order}")
    growing, grown = card_left_edge(early, geometry), card_left_edge(settled, geometry)
    checks.expect(growing is not None and grown is not None and growing > grown + 30,
                  f"the card itself still scales in: its left edge moves from {growing} to {grown}")


def main():
    Checks().run(taskbar_ready, by_default_the_backdrop_scales_in_below_the_taskbar, the_backdrop_can_cover_the_taskbar,
                 the_backdrop_can_fade_in_while_the_card_still_scales)


if __name__ == "__main__":
    main()
