#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import activate, for_window, konveyor_action, konveyor_windows, run_script, wait_for


def layout():
    return {window["title"]: tuple(window["layout"]["pos_in_scrolling_layout"]) for window in konveyor_windows()}


def set_minimized(title, minimized):
    statement = "w.minimized = true;" if minimized else "w.minimized = false; workspace.activeWindow = w;"
    run_script(for_window(title, statement))
    wait_for(lambda: (title in layout()) != minimized, 60)


def check_round_trip(title, problems):
    before = layout()
    set_minimized(title, True)
    hidden = layout()
    print(f"{title} minimized: {hidden}")
    if title in hidden:
        problems.append(f"{title} stayed in the layout after minimizing ({hidden})")
    set_minimized(title, False)
    after = layout()
    print(f"{title} restored: {after}")
    if after != before:
        problems.append(f"{title} did not return to its place: before {before}, after {after}")


def main():
    problems = []
    activate("B")
    minimizable = run_script(for_window("B", 'print("MARK|" + w.minimizable);'))
    if minimizable != ["true"]:
        problems.append(f"windows are not minimizable by default ({minimizable})")
    print("start:", layout())
    check_round_trip("B", problems)
    activate("C")
    konveyor_action("consume-or-expel-window-left")
    wait_for(lambda: layout()["C"][0] == layout()["B"][0], 60)
    print("C stacked under B:", layout())
    check_round_trip("C", problems)
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
