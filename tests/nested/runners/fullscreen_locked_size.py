#!/usr/bin/env python3
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import activate, frame, konveyor, konveyor_action, kwin_titles, reload_konveyor, wait_for, window_state
from nested import build_dir

WIDTH, HEIGHT = 1920, 1080
ROOT = Path(os.environ["KONVEYOR_TEST_ROOT"])
KWIN_LOG = Path(os.environ["KONVEYOR_KWIN_LOG"])
TARGET = re.compile(r"konveyor: target for window (\d+) .*?floating=(\w+) sizing=(\d) requested=(\d) windowedFullscreen=(\w+) kwinFullscreen=(\w+)")


def layout_window(title):
    return next((window for window in json.loads(konveyor("Windows")) if window["title"] == title), None)


def last_sizing(title):
    window = layout_window(title)
    if window is None:
        return None
    lines = [match.groups() for match in map(TARGET.search, KWIN_LOG.read_text(errors="replace").splitlines()) if match]
    mine = [line for line in lines if int(line[0]) == window["id"]]
    return int(mine[-1][2]) if mine else None


def in_grid(title):
    window = layout_window(title)
    return window is not None and not window["is_floating"]


def floating(title):
    window = layout_window(title)
    return window is not None and window["is_floating"]


def describe(title):
    window = layout_window(title)
    if window is None:
        return "gone"
    return (f"floating={window['is_floating']} slot={window['layout']['pos_in_scrolling_layout']} focused={window['is_focused']} "
            f"native={window_state(title)} sizing={last_sizing(title)}")


def rules_text():
    return "".join(path.read_text() for path in sorted((ROOT / "config" / "konveyor").glob("*")) if path.is_file())


class Game:
    def __init__(self, title, width, height):
        self.title = title
        self.app_id = "lock-" + title.lower()
        self.process = subprocess.Popen([str(build_dir() / "bin" / "fullscreen_guard_client"), title, "locked", str(width), str(height), self.app_id],
                                        stdin=subprocess.PIPE, text=True)
        if not wait_for(lambda: title in kwin_titles(), 30):
            raise RuntimeError(f"{title} never appeared")
        wait_for(lambda: layout_window(title) is not None, 30)
        time.sleep(0.5)

    def say(self, command):
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def fullscreen(self):
        self.say("fullscreen")
        wait_for(lambda: (window_state(self.title) or "").startswith("true|"), 5)
        time.sleep(0.5)

    def windowed(self, width, height):
        self.say(f"windowed {width}x{height}")
        wait_for(lambda: (window_state(self.title) or "").startswith("false|"), 5)
        time.sleep(1.5)

    def resize(self, width, height):
        self.say(f"resize {width}x{height}")
        time.sleep(1.5)

    def mod_r_works(self):
        escaped = self.app_id.replace("-", "\\-")
        before = rules_text().count(escaped)
        activate(self.title)
        konveyor_action("switch-preset-column-width")
        time.sleep(0.5)
        return rules_text().count(escaped) > before

    def close(self):
        self.process.terminate()
        self.process.wait(timeout=10)
        wait_for(lambda: self.title not in kwin_titles(), 10)


def scenario_reported_game(checks):
    width, height = WIDTH, HEIGHT
    print("== reported game: opens output-sized, real fullscreen, windowed at half size")
    game = Game("GameReported", width, height)
    print(f"opened: {describe(game.title)}")
    checks.expect(in_grid(game.title), "opened locked: in the grid, not floating")
    game.fullscreen()
    print(f"fullscreen: {describe(game.title)}")
    game.windowed(width // 2, height)
    print(f"windowed: {describe(game.title)}")
    checks.expect(in_grid(game.title), "leaving fullscreen at half the output: in the grid")
    checks.equal(last_sizing(game.title), 0, "leaving fullscreen: engine sizing mode is back to normal")
    checks.expect(game.mod_r_works(), "Mod+R acts on the windowed game")
    game.close()


def scenario_open_locked(checks):
    width, height = WIDTH, HEIGHT
    print("== locked window opening without a fullscreen request")
    neighbour = frame("A")
    game = Game("GameOpen", width, height)
    activate(game.title)
    time.sleep(0.5)
    print(f"opened: {describe(game.title)} frame={frame(game.title)} neighbour A before={neighbour} after={frame('A')}")
    window = layout_window(game.title)
    checks.expect(in_grid(game.title), "locked window opens in its column")
    checks.equal(list(frame(game.title)[:4]), [0, 0, width, height], "locked window covers the output")
    checks.expect(window["is_focused"], "locked window is focused")
    activate("A")
    time.sleep(0.5)
    print(f"focus A: {describe(game.title)} A={frame('A')}")
    activate(game.title)
    time.sleep(0.5)
    print(f"refocus game: {describe(game.title)}")
    game.close()


def scenario_fullscreen_later(checks):
    width, height = WIDTH, HEIGHT
    print("== locked window then real fullscreen one second later")
    game = Game("GameLater", width, height)
    time.sleep(1)
    game.fullscreen()
    print(f"fullscreen: {describe(game.title)}")
    checks.expect((window_state(game.title) or "").startswith("true|"), "real fullscreen granted")
    game.windowed(width, height)
    print(f"back at output size: {describe(game.title)}")
    game.close()


def scenario_shrink_without_fullscreen(checks):
    width, height = WIDTH, HEIGHT
    print("== locked window shrinks with no fullscreen step")
    game = Game("GameShrink", width, height)
    game.resize(width // 2, height)
    print(f"shrunk: {describe(game.title)}")
    checks.expect(in_grid(game.title), "shrunk without a fullscreen step: in the grid")
    game.close()


def scenario_user_float(checks):
    width, height = WIDTH, HEIGHT
    print("== user-floated fixed window through fullscreen and back")
    game = Game("GameFloat", width // 2, height // 2)
    activate(game.title)
    konveyor_action("toggle-window-floating")
    time.sleep(0.5)
    print(f"floated: {describe(game.title)}")
    checks.expect(floating(game.title), "user floated the fixed window")
    game.fullscreen()
    print(f"fullscreen: {describe(game.title)}")
    game.windowed(width // 2, height // 2)
    print(f"back: {describe(game.title)}")
    checks.expect(floating(game.title), "user-floated fixed window floats again after fullscreen")
    game.close()


def scenario_floating_resized_on_leave(checks):
    width, height = WIDTH, HEIGHT
    print("== floating fixed window leaves fullscreen at another size")
    game = Game("GameFloatSize", width // 2, height // 2)
    activate(game.title)
    konveyor_action("toggle-window-floating")
    time.sleep(0.5)
    game.fullscreen()
    game.windowed(width // 3, height // 3)
    print(f"left fullscreen at another size: {describe(game.title)}")
    checks.expect(floating(game.title), "user-floated window floats after leaving fullscreen")
    checks.equal(last_sizing(game.title), 0, "engine sizing mode is back to normal after leaving fullscreen")
    checks.expect(game.mod_r_works(), "Mod+R acts on the window that left fullscreen")
    game.close()


def scenario_restore(checks):
    width, height = WIDTH, HEIGHT
    print("== session restore of each state")
    game = Game("GameRestore", width, height)
    states = [("covering", lambda: None), ("fullscreen", game.fullscreen), ("small", lambda: game.windowed(width // 2, height))]
    for name, enter in states:
        enter()
        before = describe(game.title)
        reloaded = reload_konveyor(("A", game.title))
        time.sleep(1.5)
        after = describe(game.title)
        print(f"restore {name}: before {before}\n          after  {after} reloaded={reloaded}")
        window = layout_window(game.title)
        checks.expect(window is not None and window["is_floating"] == (before.split()[0] == "floating=True"), f"restore keeps floating state ({name})")
    game.close()


def main():
    scenarios = [scenario_reported_game, scenario_open_locked, scenario_fullscreen_later, scenario_shrink_without_fullscreen,
                 scenario_user_float, scenario_floating_resized_on_leave, scenario_restore]
    selected = [scenario for scenario in scenarios if not sys.argv[1:] or scenario.__name__ in sys.argv[1:]]
    Checks().run(*selected)


if __name__ == "__main__":
    main()
