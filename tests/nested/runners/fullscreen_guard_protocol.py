#!/usr/bin/env python3
import json
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from fakepointer import move
from kwinsession import activate, konveyor, konveyor_action, kwin_titles, qdbus, reload_konveyor, wait_for, window_minimized, window_state
from nested import build_dir

ROOT = Path(os.environ["KONVEYOR_TEST_ROOT"])
TITLE = "GuardProtocol"
LOG = ROOT / "guard-protocol.log"


def configures():
    return [line.split(":") for line in LOG.read_text().splitlines() if line.startswith("configure:")]


def settle():
    previous = configures()
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        time.sleep(0.25)
        current = configures()
        if current == previous:
            return current
        previous = current
    raise RuntimeError("configure stream did not settle")


def request(process, command, expected, problems, context):
    before = settle()
    process.stdin.write(command + "\n")
    process.stdin.flush()
    if not wait_for(lambda: len(configures()) > len(before), 3, 0.05):
        problems.append(f"{context}: no configure response")
        print(f"{context}: no configure response")
        return
    responses = settle()[len(before):]
    states = [response[2] for response in responses]
    serials = [int(response[1]) for response in responses]
    print(f"{context}: serials {serials}, states {states}, native {window_state(TITLE)}")
    if any(state != expected for state in states):
        problems.append(f"{context}: expected {expected}, received {states}")
    if len(responses) != 1:
        problems.append(f"{context}: expected one configure, received {len(responses)}")
    if len(set(serials + [int(response[1]) for response in before])) != len(serials) + len(before):
        problems.append(f"{context}: configure serial was reused")


def set_experiments(enabled):
    config = ROOT / "config" / "konveyor" / "config.kdl"
    experiments = "experiments { prevent-fullscreen-minimize; prevent-fullscreen-exit; }" if enabled else ""
    config.write_text("animations { off; }\n" + experiments)
    error = konveyor("LoadConfigFile", str(config))
    if error:
        raise RuntimeError(error)


def minimize(process, expected, problems, context):
    process.stdin.write("minimize\n")
    process.stdin.flush()
    if not wait_for(lambda: window_minimized(TITLE) == expected, 3):
        problems.append(f"{context}: minimized state did not become {expected}")
    time.sleep(0.25)
    if window_minimized(TITLE) != expected:
        problems.append(f"{context}: final minimized state differs from {expected}")
    print(f"{context}: minimized {window_minimized(TITLE)}")


def main():
    problems = []
    outputs = json.loads(konveyor("Outputs"))
    if len(outputs) > 1:
        move(2880, 540)
    with LOG.open("w") as log:
        process = subprocess.Popen([str(build_dir() / "bin" / "fullscreen_guard_client"), TITLE, "protocol"],
                                   stdin=subprocess.PIPE, stdout=log, stderr=subprocess.STDOUT, text=True)
    if not wait_for(lambda: TITLE in kwin_titles(), 30):
        raise RuntimeError("protocol client never mapped")
    activate(TITLE)
    request(process, "unset", "normal", problems, "guards off: already normal")
    request(process, "unset", "normal", problems, "guards off: repeated unset")
    request(process, "fullscreen", "fullscreen", problems, "guards off: enter fullscreen")
    activate("A")
    request(process, "unset", "normal", problems, "guards off: inactive exit")
    set_experiments(True)
    activate(TITLE)
    error = konveyor_action("toggle-window-floating")
    if error:
        raise RuntimeError(error)
    settle()
    floating_frame = window_state(TITLE)
    request(process, "fullscreen-output", "fullscreen", problems, "guards on: enter fullscreen on requested output")
    fullscreen_frame = window_state(TITLE)
    activate("A")
    print(f"guards on: before denied exit: {window_state(TITLE)}")
    request(process, "unset", "fullscreen", problems, "guards on: denied exit")
    request(process, "unset", "fullscreen", problems, "guards on: repeated denied exit")
    if window_state(TITLE) != fullscreen_frame:
        problems.append(f"denied exit changed output/frame from {fullscreen_frame} to {window_state(TITLE)}")
    minimize(process, False, problems, "guards on: deny minimize")
    activate(TITLE)
    request(process, "unset", "normal", problems, "guards on: active exit")
    if window_state(TITLE) != floating_frame:
        problems.append(f"denied exits lost floating restore geometry from {floating_frame} to {window_state(TITLE)}")
    request(process, "unset", "normal", problems, "guards on: repeated normal unset")
    set_experiments(False)
    request(process, "unset", "normal", problems, "guards disabled: repeated unset")
    request(process, "fullscreen", "fullscreen", problems, "guards disabled: enter fullscreen")
    minimize(process, True, problems, "guards disabled: accept fullscreen minimize")
    activate(TITLE)
    activate("A")
    request(process, "unset", "normal", problems, "guards disabled: inactive exit after minimize/restore")
    request(process, "unset", "normal", problems, "guards disabled: repeated unset after minimize/restore")
    activate(TITLE)
    set_experiments(True)
    if not reload_konveyor(("A", TITLE)):
        problems.append("reload did not manage existing windows")
    activate(TITLE)
    request(process, "fullscreen", "fullscreen", problems, "reload: enter fullscreen")
    activate("A")
    request(process, "unset", "fullscreen", problems, "reload: denied exit")
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.unloadEffect", "konveyor_effect")
    request(process, "unset", "normal", problems, "unloaded: inactive exit")
    request(process, "unset", "normal", problems, "unloaded: repeated normal unset")
    minimize(process, True, problems, "unloaded: accept minimize")
    process.terminate()
    process.wait(timeout=10)
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
