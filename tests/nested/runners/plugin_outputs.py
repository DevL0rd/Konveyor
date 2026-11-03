#!/usr/bin/env python3
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, default_config, load_config
from fakepointer import move
from kwinsession import (
    activate,
    konveyor_action,
    managed_titles,
    qdbus,
    reload_konveyor,
    wait_for,
)
from multimonitor_ownership import (
    active_output,
    engine_output,
    green_columns_on,
    outputs,
    spill_b_onto,
)


def without_border_styles():
    return re.sub(
        r"^\s*(active|inactive|urgent)-style .*\n", "", default_config(), flags=re.M
    )


def spilled_window_without_border_styles(checks):
    checks.expect(
        load_config(without_border_styles()), "a config without border styles loads"
    )
    screens = outputs()
    left, right = sorted(screens, key=lambda name: screens[name][0])
    problems = []
    spill_b_onto(left, right, problems)
    checks.equal(problems, [], "B spills onto the other output")
    checks.expect(
        wait_for(lambda: green_columns_on(*screens[left]), 10),
        "the part of B on its own output is drawn",
    )


def window_activated_from_the_other_output(checks):
    screens = outputs()
    left, right = sorted(screens, key=lambda name: screens[name][0])
    move(screens[left][0] + 400, 540)
    checks.expect(activate("A"), "A on the first output is active")
    checks.expect(
        activate("C"),
        "C on the second output is activated while the pointer is on the first",
    )
    checks.expect(
        wait_for(lambda: active_output() == right),
        f"the active output follows the window's home output ({active_output()})",
    )


def output_unplugged(checks):
    screens = outputs()
    left, right = sorted(screens, key=lambda name: screens[name][0])
    subprocess.run(
        ["kscreen-doctor", f"output.{right}.disable"], capture_output=True, text=True
    )
    checks.expect(
        wait_for(lambda: len(outputs()) == 1, 60), "the second output went away"
    )
    checks.expect(
        wait_for(lambda: len(managed_titles()) == 3),
        "all three windows are still managed",
    )
    subprocess.run(
        ["kscreen-doctor", f"output.{right}.enable"], capture_output=True, text=True
    )
    checks.expect(wait_for(lambda: len(outputs()) == 2, 60), "the output came back")


def reloaded_without_the_output(checks):
    screens = outputs()
    left, right = sorted(screens, key=lambda name: screens[name][0])
    activate("C")
    konveyor_action("move-column-to-monitor", right)
    checks.expect(
        wait_for(lambda: engine_output("C") == right),
        f"C lives on {right} ({engine_output('C')})",
    )
    titles = managed_titles()
    qdbus(
        "org.kde.KWin",
        "/Effects",
        "org.kde.kwin.Effects.unloadEffect",
        "konveyor_effect",
    )
    subprocess.run(
        ["kscreen-doctor", f"output.{right}.disable"], capture_output=True, text=True
    )
    checks.expect(
        wait_for(lambda: len(outputs()) == 1, 60),
        "the second output went away while the effect was unloaded",
    )
    checks.expect(
        reload_konveyor(titles), "the effect loads again and manages the windows"
    )
    subprocess.run(
        ["kscreen-doctor", f"output.{right}.enable"], capture_output=True, text=True
    )
    checks.expect(wait_for(lambda: len(outputs()) == 2, 60), "the output came back")


def main():
    Checks().run(
        spilled_window_without_border_styles,
        window_activated_from_the_other_output,
        output_unplugged,
        reloaded_without_the_output,
    )


if __name__ == "__main__":
    main()
