#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, config_path
from fakepointer import move
from kwinsession import konveyor, konveyor_action, wait_for

HOME = (960, 540)
TOP_LEFT = (0, 0)
TOP_RIGHT = (3839, 0)


def overview_open():
    return json.loads(konveyor("OverviewState"))["is_open"]


def push(corner):
    move(*HOME, settle=False)
    move(*corner, settle=False)


def push_until(corner, wanted):
    for _ in range(5):
        push(corner)
        if wait_for(lambda: overview_open() == wanted, 6):
            return True
    return False


def reserved_corner(checks):
    checks.equal(overview_open(), False, "the overview starts closed")
    checks.expect(push_until(TOP_LEFT, True), "the top-left hot corner opens the overview")
    checks.expect(push_until(TOP_LEFT, False), "the top-left hot corner closes it again")


def per_output_override(checks):
    push(TOP_RIGHT)
    checks.equal(overview_open(), False, "the top-right corner of Virtual-1, where hot corners are off, does nothing")
    checks.expect(push_until(TOP_LEFT, True), "the top-left corner of Virtual-0 still opens the overview")
    konveyor_action("close-overview")
    checks.expect(wait_for(lambda: not overview_open()), "close-overview closes it")


def switched_off(checks):
    text = config_path().read_text().replace("top-left\n        top-right", "off")
    config_path().write_text(text)
    checks.equal(konveyor("LoadConfigFile", ""), "", "the config reloads")
    konveyor_action("open-overview")
    checks.expect(wait_for(overview_open), "open-overview opens the overview")
    push(TOP_LEFT)
    checks.equal(overview_open(), True, "with hot corners off the corner no longer toggles the overview")
    konveyor_action("toggle-overview")
    checks.expect(wait_for(lambda: not overview_open()), "toggle-overview closes it")


def main():
    Checks().run(reserved_corner, per_output_override, switched_off)


if __name__ == "__main__":
    main()
