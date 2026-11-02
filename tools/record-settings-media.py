#!/usr/bin/env python3
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
NESTED = REPO / "tests" / "nested"
sys.path[:0] = [str(NESTED), str(NESTED / "harness"), str(NESTED / "runners")]

PAGES = ["layout", "appearance", "motion", "mouse", "touch", "shortcuts", "rules", "monitors", "workspaces", "taskbar", "plasma", "experiments"]
GIF_PAGES = ["motion", "mouse", "touch", "shortcuts", "rules", "monitors", "workspaces", "taskbar", "plasma"]
RAIL_X, RAIL_TOP, RAIL_STEP = 160, 140, 44
TAB_Y = 371
TABS = {"focus-ring": 693, "animated-styles": 857, "event-effects": 964}
NEW_WINDOWS_NOTCHES = 7
REMEMBER_NOTCHES = 7
GIF_SIZE = (960, 599)
GIF_FPS = 10
STILL_SIZE = (1200, 710)
STILL_BOX = (94, 66, 1616, 966)
SCHEME = "/usr/share/color-schemes/BreezeDark.colors"
USER = """import QtQuick

QtObject {
    readonly property url faceIconUrl: ""
    readonly property string fullName: "DevL0rd"
    readonly property string loginName: "devl0rd"
    readonly property string host: "workstation"
}
"""


def outer(arguments):
    import test_kontrol_panel as panel_test
    from nested import run_runner

    work = Path(arguments.work).resolve()
    shutil.rmtree(work, ignore_errors=True)
    (work / "frames").mkdir(parents=True)
    with tempfile.TemporaryDirectory(prefix="konveyor-settings-media-") as temporary:
        panel, imports, program = panel_test.stage(Path(temporary))
        (imports / "org" / "kde" / "coreaddons" / "KUser.qml").write_text(USER)
        local = imports / "konveyor-kontrol-panel"
        shutil.copy(program, local)
        files = {
            "data/applications/konveyor-kontrol-panel.desktop": panel_test.PROGRAM_DESKTOP.format(program=local, panel=panel),
            "config/kdeglobals": "[General]\nColorScheme=BreezeDark\nAccentColor=61,174,233\n\n" + Path(SCHEME).read_text(),
            "config/plasmarc": "[Theme]\nname=breeze-dark\n",
        }
        setup = "export QT_QPA_PLATFORMTHEME=kde\nexport QT_STYLE_OVERRIDE=Breeze\n" + panel_test.SETUP.format(imports=imports, panel=panel)
        code = run_runner(Path(__file__).resolve(), clients=(), timeout=420, setup=setup, files=files, extra_kwinrc=panel_test.NORMAL_FOCUS_STEALING_PREVENTION,
                          permission_checks=True, global_shortcuts=True, width=1920, height=1200,
                          arguments=f"--inside --work {work} --output {Path(arguments.output).resolve()} --program {local}")
    if code == 0:
        encode(work, Path(arguments.output).resolve())
    return code


def encode(work, output):
    palette = "split[a][b];[a]palettegen=max_colors=64:stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=5:diff_mode=rectangle"
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-framerate", str(GIF_FPS), "-i", str(work / "frames" / "%04d.png"), "-vf", palette, "-loop", "0",
                    str(output / "settings-tour.gif")], check=True)
    for path in sorted(output.glob("settings-*.*")):
        print(path.name, path.stat().st_size)


class Pointer:
    def __init__(self):
        from fakepointer import binary
        from sessionguard import popen

        self.process = popen([str(binary()), "stdin"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
        self.position = (0.0, 0.0)

    def send(self, token):
        self.process.stdin.write(token + "\n")
        self.process.stdin.flush()
        self.process.stdout.readline()

    def move(self, x, y):
        self.position = (x, y)
        self.send(f"move:{x:.1f}:{y:.1f}")

    def click(self):
        self.send("button:272:1")
        time.sleep(0.06)
        self.send("button:272:0")

    def close(self):
        self.process.stdin.close()
        self.process.wait(timeout=10)


class Screen:
    def __init__(self, card_box):
        import dbus

        self.interface = dbus.Interface(dbus.SessionBus().get_object("org.kde.KWin.ScreenShot2", "/org/kde/KWin/ScreenShot2"), "org.kde.KWin.ScreenShot2")
        self.box = card_box

    def grab(self):
        from screenshot import read_workspace

        return read_workspace(self.interface).convert("RGB").crop(self.box)


def cursor(scale):
    from PIL import Image, ImageDraw

    outline = [(0, 0), (0, 17), (4.2, 13.2), (7, 19.5), (9.8, 18.3), (7.1, 12.2), (12.5, 12)]
    size = 4
    sprite = Image.new("RGBA", (int(16 * scale * size), int(22 * scale * size)), (0, 0, 0, 0))
    draw = ImageDraw.Draw(sprite)
    points = [(x * scale * size + 2 * size, y * scale * size + 2 * size) for x, y in outline]
    draw.polygon(points, fill=(255, 255, 255, 255), outline=(15, 15, 15, 255), width=size)
    return sprite.resize((sprite.width // size, sprite.height // size), Image.LANCZOS)


def smooth(t):
    return t * t * (3 - 2 * t)


class Director:
    def __init__(self, pointer, screen, origin):
        self.pointer = pointer
        self.screen = screen
        self.origin = origin
        self.frames = []
        self.cursor = cursor(1.0)

    def absolute(self, x, y):
        return self.origin[0] + x, self.origin[1] + y

    def frame(self, started):
        image = self.screen.grab()
        x, y = self.pointer.position
        self.frames.append((time.monotonic() - started, image, (x - self.origin[0], y - self.origin[1])))

    def travel(self, target, duration, recording=None):
        start = self.pointer.position
        goal = self.absolute(*target)
        began = time.monotonic()
        while True:
            fraction = min(1.0, (time.monotonic() - began) / duration) if duration else 1.0
            eased = smooth(fraction)
            self.pointer.move(start[0] + (goal[0] - start[0]) * eased, start[1] + (goal[1] - start[1]) * eased)
            if recording is not None:
                self.frame(recording)
            if fraction >= 1.0:
                return

    def wait(self, duration, recording=None):
        end = time.monotonic() + duration
        while time.monotonic() < end:
            if recording is not None:
                self.frame(recording)
            else:
                time.sleep(0.05)

    def go(self, target, travel, settle, click=False, recording=None):
        self.travel(target, travel, recording)
        if click:
            self.pointer.click()
        self.wait(settle, recording)

    def resampled(self):
        from PIL import Image

        total = self.frames[-1][0]
        count = int(total * GIF_FPS)
        picked = []
        index = 0
        for tick in range(count):
            while index + 1 < len(self.frames) and self.frames[index + 1][0] <= tick / GIF_FPS:
                index += 1
            picked.append(self.frames[index])
        scale = GIF_SIZE[0] / self.frames[0][1].width
        sprite = self.cursor.resize((max(1, round(self.cursor.width * scale * 1.5)), max(1, round(self.cursor.height * scale * 1.5))), Image.LANCZOS)
        for number, (_, image, (x, y)) in enumerate(picked):
            small = image.resize(GIF_SIZE, Image.LANCZOS).convert("RGBA")
            small.alpha_composite(sprite, (round(x * scale), round(y * scale)))
            yield number, small.convert("RGB")


def rail(page):
    return RAIL_X, RAIL_TOP + RAIL_STEP * PAGES.index(page)


def visit(director, page, wait):
    director.go(rail(page), 0.0, wait, click=True)


def save_still(director, name, box=STILL_BOX, size=STILL_SIZE):
    from PIL import Image

    image = director.screen.grab().crop(box).resize(size, Image.LANCZOS)
    image.save(director.output / f"settings-{name}.png", optimize=True)


def type_text(text):
    from fakepointer import keys
    from keycodes import KEY_CODES

    for letter in text:
        code = KEY_CODES[letter.upper()]
        keys((code, 1), (code, 0))
        time.sleep(0.08)


def warm(director):
    for page in PAGES:
        visit(director, page, 5.0)
    director.go((TABS["focus-ring"], TAB_Y), 0.0, 0.5)
    visit(director, "appearance", 3.0)
    director.go((TABS["focus-ring"], TAB_Y), 0.0, 1.0, click=True)
    visit(director, "layout", 4.0)


def scroll(director, notches):
    director.go((560, 600), 0.0, 0.3)
    for _ in range(abs(notches)):
        director.pointer.send(f"axis:0:{15 if notches > 0 else -15}")
        time.sleep(0.12)
    time.sleep(1.2)


def stills(director):
    for page in PAGES:
        visit(director, page, 3.0)
        if page in ("taskbar", "experiments"):
            continue
        save_still(director, page)
        if page == "layout":
            scroll(director, NEW_WINDOWS_NOTCHES)
            save_still(director, "new-windows")
            scroll(director, REMEMBER_NOTCHES)
            save_still(director, "remember")
            scroll(director, -(NEW_WINDOWS_NOTCHES + REMEMBER_NOTCHES + 4))
    visit(director, "appearance", 2.0)
    director.go((TABS["animated-styles"], TAB_Y), 0.0, 2.5, click=True)
    save_still(director, "appearance")
    director.go((TABS["focus-ring"], TAB_Y), 0.0, 1.5, click=True)
    visit(director, "layout", 2.0)


def tour(director):
    started = time.monotonic()
    director.pointer.move(*director.absolute(1000, 760))
    director.wait(0.5, started)
    director.go(rail("layout"), 0.5, 0.1, recording=started)
    director.go(rail("appearance"), 0.4, 0.7, click=True, recording=started)
    director.go((450, 240), 0.6, 0.6, recording=started)
    director.go((TABS["animated-styles"], TAB_Y), 0.6, 0.7, click=True, recording=started)
    director.go((645, 545), 0.5, 0.5, recording=started)
    director.go((970, 545), 0.4, 0.3, recording=started)
    director.go((TABS["event-effects"], TAB_Y), 0.6, 1.3, click=True, recording=started)
    for page in GIF_PAGES:
        director.go(rail(page), 0.25, 0.35, click=True, recording=started)
    director.go(rail("layout"), 0.6, 0.5, click=True, recording=started)


def extras(director):
    visit(director, "monitors", 3.0)
    director.go((700, 470), 0.0, 3.0, click=True)
    save_still(director, "portrait")
    director.go((814, 36), 0.0, 0.8, click=True)
    type_text("width")
    time.sleep(3.0)
    save_still(director, "hub", (0, 0, 1628, 597), (1200, 439))


def inside(arguments):
    import dbus
    from kontrol_panel import allow_fake_input, card, is_open, panel
    from kwinsession import wait_for
    from sessionguard import popen

    output = Path(arguments.output)
    work = Path(arguments.work)
    assert allow_fake_input()
    service = popen([arguments.program, os.environ["KONVEYOR_KONTROL_PANEL_DIR"]], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    assert wait_for(lambda: dbus.SessionBus().name_has_owner("org.devl0rd.KontrolPanel"), 60)
    panel("Open", "settings")
    assert wait_for(is_open, 30)
    time.sleep(8)
    _, (x, y, width, height) = card()
    origin = (round(x), round(y))
    pointer = Pointer()
    director = Director(pointer, Screen((origin[0], origin[1], origin[0] + round(width), origin[1] + round(height))), origin)
    director.output = output
    try:
        warm(director)
        stills(director)
        tour(director)
        extras(director)
        for number, image in director.resampled():
            image.save(work / "frames" / f"{number:04d}.png", compress_level=1)
    finally:
        pointer.close()
        service.terminate()
        service.wait(timeout=30)
    print("RESULT: PASS")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--inside", action="store_true")
    parser.add_argument("--work", default=str(REPO / "tmp" / "borders" / "media" / "work"))
    parser.add_argument("--output", default=str(REPO / "docs" / "media"))
    parser.add_argument("--program")
    arguments = parser.parse_args()
    if arguments.inside:
        return inside(arguments)
    return outer(arguments)


if __name__ == "__main__":
    sys.exit(main())
