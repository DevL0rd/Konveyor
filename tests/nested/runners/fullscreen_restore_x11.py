#!/usr/bin/env python3
import json
import os
import sys
from pathlib import Path

from Xlib import X, Xutil, display

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from x11window import send_fullscreen
from kwinsession import activate, for_window, konveyor, konveyor_action, kwin_titles, reload_konveyor, run_script, wait_for, watch_changes, window_state
from screenshot import capture_workspace, close

TITLE = "X11ModeChanges"


def layout_window():
    return next(window for window in json.loads(konveyor("Windows")) if window["title"] == TITLE)


def columns():
    workspace = layout_window()["workspace_id"]
    return next(state["columns"] for state in json.loads(konveyor("Workspaces")) if state["id"] == workspace)


def main():
    checks = Checks()
    conn = display.Display()
    screen = conn.screen()
    width, height = screen.width_in_pixels, screen.height_in_pixels
    fullscreen = f"true|0,0 {width}x{height}"
    windowed = f"false|0,0 {width}x{height}"
    window = screen.root.create_window(0, 0, 800, 600, 0, screen.root_depth, X.InputOutput, X.CopyFromParent,
                                       background_pixel=screen.white_pixel, event_mask=X.PropertyChangeMask | X.StructureNotifyMask | X.ExposureMask)
    window.set_wm_name(TITLE)
    window.set_wm_class("x11modechanges", "X11ModeChanges")
    window.set_wm_normal_hints(flags=Xutil.PMinSize | Xutil.PMaxSize, min_width=800, min_height=600, max_width=800, max_height=600)
    window.map()
    conn.sync()
    if not checks.expect(wait_for(lambda: TITLE in kwin_titles(), 30), "X11 game appeared"):
        checks.run()
        return
    activate(TITLE)
    send_fullscreen(conn, window, True)
    checks.expect(wait_for(lambda: window_state(TITLE) == fullscreen, 10), "game entered fullscreen")
    window.set_wm_normal_hints(flags=Xutil.PMinSize | Xutil.PMaxSize, min_width=width, min_height=height, max_width=width, max_height=height)
    conn.sync()
    original = columns()
    for focused in (TITLE, "A", "C", "B"):
        activate(focused)
        send_fullscreen(conn, window, False)
        checks.expect(wait_for(lambda: window_state(TITLE).startswith("false|"), 10), f"{focused}: protocol fullscreen removal accepted")
        window.configure(x=0, y=0, width=width, height=height)
        conn.sync()
        checks.expect(wait_for(lambda: window_state(TITLE) == windowed, 10),
                      f"{focused}: inactive and active protocol exits retain output geometry")
        checks.expect(not layout_window()["is_floating"], f"{focused}: game remains tiled after protocol exit")
        print(f"{focused}: exit state {window_state(TITLE)}")
        send_fullscreen(conn, window, True)
        checks.expect(wait_for(lambda: window_state(TITLE) == fullscreen, 10), f"{focused}: game returned fullscreen")
        checks.equal(columns(), original, f"{focused}: original column order restored")
    activate(TITLE)
    send_fullscreen(conn, window, False)
    wait_for(lambda: window_state(TITLE).startswith("false|"), 10)
    window.configure(x=0, y=0, width=width, height=height)
    conn.sync()
    checks.expect(wait_for(lambda: window_state(TITLE) == windowed, 10), "persistent protocol exit accepts client geometry")
    activate("A")
    checks.equal(watch_changes(TITLE, 1), [], "focus changes leave the windowed game's geometry untouched")
    checks.equal(window_state(TITLE), windowed, "windowed game remains on its client-chosen frame behind tiles")
    image = capture_workspace(str(Path(os.environ["KONVEYOR_REPORT"]).with_suffix(".overlay.png")))
    frame = next(state for state in json.loads(konveyor("Windows")) if state["title"] == "A")["layout"]
    x, y = frame["tile_pos_in_workspace_view"]
    tile_width, tile_height = frame["window_size"]
    checks.expect(close(image.getpixel((int(x + tile_width / 2), int(y + tile_height / 2))), (47, 48, 51, 255)),
                  "focused tile renders above persistently windowed output-sized game")
    activate(TITLE)
    def game_is_above_tiles():
        game = int(run_script(for_window(TITLE, 'print("MARK|" + w.stackingOrder);'))[0])
        return all(game > int(run_script(for_window(title, 'print("MARK|" + w.stackingOrder);'))[0]) for title in ("A", "B", "C"))

    checks.expect(wait_for(game_is_above_tiles, 10), "refocused windowed game stacks above its overlaid tiles")
    activate("C")
    checks.equal(watch_changes(TITLE, 1), [], "Konveyor does not reassert output coordinates on focus")
    checks.equal(window_state(TITLE), windowed, "client-selected coordinates remain untouched")
    position = layout_window()["layout"]["pos_in_scrolling_layout"]
    run_script(for_window(TITLE, "w.minimized = true;"))
    run_script(for_window(TITLE, "w.minimized = false;"))
    checks.expect(wait_for(lambda: any(item["title"] == TITLE for item in json.loads(konveyor("Windows"))), 10),
                  "unminimized game is tracked again")
    checks.expect(not layout_window()["is_floating"], "unminimized game keeps tiled presentation")
    checks.equal(layout_window()["layout"]["pos_in_scrolling_layout"], position, "unminimized game keeps its tile slot")
    checks.expect(reload_konveyor([TITLE, "A", "B", "C"]), "effect reload adopts all windows")
    checks.expect(not layout_window()["is_floating"], "effect reload keeps game tiled")
    checks.equal(layout_window()["layout"]["pos_in_scrolling_layout"], position, "effect handoff retains tile slot")
    checks.equal(window_state(TITLE), windowed, "effect reload leaves actual windowed mode and geometry intact")
    window.set_wm_normal_hints(flags=0)
    conn.sync()
    activate(TITLE)
    send_fullscreen(conn, window, False)
    checks.expect(wait_for(lambda: window_state(TITLE).startswith("false|"), 10), "unlocked game accepts windowed mode")
    if layout_window()["is_floating"]:
        checks.equal(konveyor_action("toggle-window-floating"), "", "return unlocked game to tiling")
    checks.equal(konveyor_action("set-column-width", "50%"), "", "request a regular column width after clearing size hints")
    checks.expect(wait_for(lambda: layout_window()["layout"]["window_size"][0] < width, 10),
                  "cleared size hints allow regular column width instead of stale fullscreen width")
    print(f"unlocked tiled state: {window_state(TITLE)}, {layout_window()}")
    checks.run()
    window.destroy()
    conn.sync()
    conn.close()


if __name__ == "__main__":
    main()
