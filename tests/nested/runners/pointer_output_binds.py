#!/usr/bin/env python3
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from checks import Checks, default_config, load_config
from fakepointer import chord, move, tap
from keycodes import KEY_CODES, MODIFIER_CODES
from kwinsession import activate, konveyor, wait_for
from multimonitor_focus import engine_focus, engine_output, focus, open_client, outputs, perform, placed

SUPER = MODIFIER_CODES["Super"]
WHEEL_RIGHT = "axis:1:15"


def active_workspace(output):
    return next(workspace["idx"] for workspace in json.loads(konveyor("Workspaces")) if workspace["output"] == output and workspace["is_active"])


def screens():
    named = outputs()
    names = sorted(named, key=lambda name: named[name][0])
    return names[0], names[1], named


def settle_focus(title):
    return wait_for(lambda: engine_focus() == title and placed(title), 30)


def build(checks):
    left, right, named = screens()
    move(named[left][0] + 400, 540, settle=False)
    for title in ("A", "A2"):
        open_client(title)
        checks.expect(wait_for(lambda: placed(title) and engine_focus() == title, 60), f"{title} opens on {left}")
    for title in ("B", "D"):
        move(named[right][0] + 400, 540, settle=False)
        open_client(title)
        checks.expect(wait_for(lambda: placed(title) and engine_focus() == title, 60), f"{title} opens on {right}")
    focus("B")
    checks.expect(wait_for(lambda: engine_output() == right), f"B holds the focus on {right}")


def key_column_actions_follow_the_focused_window(checks):
    left, right, named = screens()
    focus("A")
    checks.expect(settle_focus("A"), f"A is the first column of {left} ({engine_focus()})")
    move(named[right][0] + 400, 540)
    tap([SUPER], KEY_CODES["Right"])
    checks.expect(wait_for(lambda: engine_focus() == "A2", 15), f"Meta+Right with the pointer on {right} still walks the focused window's row ({engine_focus()})")


def wheel_scrolls_columns_under_the_pointer(checks):
    left, right, named = screens()
    focus("B")
    focus("A")
    move(named[right][0] + 400, 540)
    chord([SUPER], WHEEL_RIGHT)
    checks.expect(wait_for(lambda: engine_focus() == "D", 15), f"Meta+wheel right scrolls the columns of {right}, where the pointer is ({engine_focus()})")
    checks.equal(engine_output(), right, "and the layout's output follows it")


def key_column_actions_follow_the_pointer_when_asked(checks):
    left, right, named = screens()
    checks.expect(load_config(default_config().replace("    // focus-keys-follow-mouse\n", "    focus-keys-follow-mouse\n")), "focus-keys-follow-mouse loads")
    focus("B")
    focus("A")
    checks.expect(settle_focus("A"), f"A is focused on {left} ({engine_focus()})")
    move(named[right][0] + 400, 540)
    tap([SUPER], KEY_CODES["Right"])
    checks.expect(wait_for(lambda: engine_focus() == "D", 15), f"Meta+Right with the pointer on {right} walks {right}'s row ({engine_focus()})")
    checks.equal(engine_output(), right, "and the layout's output follows it")
    checks.expect(load_config(default_config()), "the default config loads again")
    focus("A")
    checks.expect(settle_focus("A"), f"A is focused on {left} again ({engine_focus()})")
    move(named[right][0] + 400, 540)
    tap([SUPER], KEY_CODES["Right"])
    checks.expect(wait_for(lambda: engine_focus() == "A2", 15), f"with the option off Meta+Right walks the focused window's row again ({engine_focus()})")


def workspace_keys_switch_the_pointer_output(checks):
    left, right, named = screens()
    focus("B")
    perform("focus-workspace-down")
    checks.expect(wait_for(lambda: active_workspace(right) == 2), f"{right} shows its empty workspace ({active_workspace(right)})")
    focus("A")
    move(named[right][0] + 400, 540)
    left_before = active_workspace(left)
    tap([SUPER], KEY_CODES["1"])
    checks.expect(wait_for(lambda: active_workspace(right) == 1, 15), f"Meta+1 with the pointer on empty {right} goes to its first workspace ({active_workspace(right)})")
    checks.equal(active_workspace(left), left_before, f"{left} keeps its workspace")
    perform("focus-workspace-down")
    checks.expect(wait_for(lambda: active_workspace(right) == 2), f"{right} is on its empty workspace again ({active_workspace(right)})")
    activate("A")
    move(named[right][0] + 400, 540)
    tap([SUPER], KEY_CODES["Page_Up"])
    checks.expect(wait_for(lambda: active_workspace(right) == 1, 15), f"Meta+Page_Up with the pointer on empty {right} goes up on {right} ({active_workspace(right)})")
    checks.equal(active_workspace(left), left_before, f"{left} keeps its workspace")
    move(named[left][0] + 400, 540)
    tap([SUPER], KEY_CODES["1"])
    checks.expect(wait_for(lambda: engine_output() == left, 15), "Meta+1 with the pointer on the focused window's output acts there")


def main():
    Checks().run(build, key_column_actions_follow_the_focused_window, wheel_scrolls_columns_under_the_pointer,
                 key_column_actions_follow_the_pointer_when_asked, workspace_keys_switch_the_pointer_output)


if __name__ == "__main__":
    main()
