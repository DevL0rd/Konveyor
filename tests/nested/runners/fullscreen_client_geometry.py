#!/usr/bin/env python3
import json
import subprocess
import sys
import time
from pathlib import Path

from Xlib import X, Xutil, display

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from x11window import send_fullscreen
from kwinsession import CLIENTS, activate, frame, konveyor, konveyor_action, kwin_titles, run_script, wait_for, watch_changes, window_state

TITLE = "X11ClientGeometry"


def layout_window():
    return next(window for window in json.loads(konveyor("Windows")) if window["title"] == TITLE)


def main():
    checks = Checks()
    panel = subprocess.Popen(["qml6", str(CLIENTS / "top-panel.qml")])
    conn = display.Display()
    screen = conn.screen()
    width, height = screen.width_in_pixels, screen.height_in_pixels

    def workarea():
        lines = run_script('const a = workspace.clientArea(KWin.MaximizeArea, workspace.activeWindow); '
                           'print("MARK|" + [a.x, a.y, a.width, a.height].join(","));')
        return [int(float(value)) for value in lines[0].split(",")] if lines else None

    checks.expect(wait_for(lambda: workarea() == [0, 46, width, height - 46], 10), "Wayland panel establishes exact 46px KWin work area")
    checks.expect(wait_for(lambda: frame("A")[1] >= 46, 10), "ordinary tiled windows respect panel work area")
    window = screen.root.create_window(0, 0, 800, 600, 0, screen.root_depth, X.InputOutput, X.CopyFromParent,
                                       background_pixel=screen.white_pixel, event_mask=X.StructureNotifyMask)
    window.set_wm_name(TITLE)
    window.set_wm_class("x11clientgeometry", "X11ClientGeometry")
    window.set_wm_normal_hints(flags=Xutil.PMinSize | Xutil.PMaxSize, min_width=800, min_height=600, max_width=800, max_height=600)
    window.map()
    conn.sync()
    checks.expect(wait_for(lambda: TITLE in kwin_titles(), 30), "fixed-size X11 client appeared")
    activate(TITLE)
    time.sleep(1)
    saved = frame(TITLE)
    position = layout_window()["layout"]["pos_in_scrolling_layout"]
    send_fullscreen(conn, window, True)
    fullscreen = f"true|0,0 {width}x{height}"
    checks.expect(wait_for(lambda: window_state(TITLE) == fullscreen, 10), "native fullscreen covers output despite panel")
    window.set_wm_normal_hints(flags=Xutil.PMinSize | Xutil.PMaxSize, min_width=width, min_height=height, max_width=width, max_height=height)
    conn.sync()
    send_fullscreen(conn, window, False)
    expected_restored = f"false|{int(saved[0])},{int(saved[1])} {width}x{height}"
    checks.expect(wait_for(lambda: window_state(TITLE) == expected_restored, 10), "uncorrected REMOVE exposes KWin saved origin with current fixed size")
    print(f"native REMOVE without client correction: saved={saved}, actual={window_state(TITLE)}")
    checks.expect(not layout_window()["is_floating"], "output-sized client remains tiled after native REMOVE")
    checks.equal(layout_window()["layout"]["pos_in_scrolling_layout"], position, "native REMOVE preserves original tile slot")
    activate("A")
    checks.equal(watch_changes(TITLE, 1), [], "focus causes no corrective client geometry writes")
    checks.equal(window_state(TITLE), expected_restored, "saved native restore origin remains observable after focus")
    window.configure(x=0, y=46)
    conn.sync()
    client_selected = f"false|0,46 {width}x{height}"
    checks.expect(wait_for(lambda: window_state(TITLE) == client_selected, 10), "explicit client work-area origin remains authoritative")
    activate(TITLE)
    checks.equal(window_state(TITLE), client_selected, "refocus preserves explicit client origin")
    window.set_wm_normal_hints(flags=0)
    conn.sync()
    checks.equal(konveyor_action("set-column-width", "50%"), "", "ordinary windowed size remains available after releasing hints")
    checks.expect(wait_for(lambda: frame(TITLE)[2] < width, 10), "intentional smaller windowed mode is honored")
    print(f"workarea={workarea()}, windowed={window_state(TITLE)}")
    window.destroy()
    conn.sync()
    conn.close()
    panel.terminate()
    panel.wait(timeout=10)
    checks.run()


if __name__ == "__main__":
    main()
