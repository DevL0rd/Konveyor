#!/usr/bin/env python3
import re
import sys
import tempfile
import os
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from accessibility import Accessibility
from checks import Checks, config_path, default_config, load_config
from kwinsession import activate, frame, wait_for
from screenshot import capture_workspace

BAND = 10


def band(title):
    x, y, width, height = (round(value) for value in frame(title))
    path = tempfile.mktemp(suffix=".png", dir=os.environ["KONVEYOR_TEST_ROOT"])
    image = capture_workspace(path)
    return image.crop((x - BAND, y, x, y + height)).tobytes()


def written():
    return config_path().read_text()


def starting_point(checks):
    checks.expect(load_config(default_config()), "the default config loads")
    activate("B")
    accessibility = Accessibility()
    try:
        settings = accessibility.launch("kcmshell6", "kcm_konveyor", "--args", "appearance")
        try:
            checks.expect(accessibility.wait_for_application("kcmshell6"), "the Appearance page opens")
            application = accessibility.application("kcmshell6")
            checks.expect(wait_for(lambda: accessibility.buttons_after(application, "Starting points", "Use on"), 30, 0.5), "the starting points offer a Use button")
            buttons = accessibility.buttons_after(application, "Starting points", "Use on")
            activate("B")
            time.sleep(1)
            before = band("B")
            checks.expect(re.search(r'active-style "neon-sweep"', written()) is None, "the config does not use Neon sweep yet")
            accessibility.press(buttons[0])
            checks.expect(wait_for(lambda: re.search(r'active-style "neon-sweep"', written()), 20, 0.5), "Use on the first starting point writes it as the focus ring style")
            checks.expect('border-style "neon-sweep"' in written(), "the starting point is saved as a border style of its own")
            activate("B")
            checks.expect(wait_for(lambda: band("B") != before, 20, 0.5), "the focused window's frame changed to the new style")
            first = band("B")
            checks.expect(wait_for(lambda: band("B") != first, 10, 0.3), "the new frame keeps moving")
        finally:
            settings.terminate()
            settings.wait(timeout=10)
    finally:
        accessibility.close()


def main():
    Checks().run(starting_point)


if __name__ == "__main__":
    main()
