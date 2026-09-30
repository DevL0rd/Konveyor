#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path

import dbus

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import CLIENTS, konveyor_windows, managed_titles, wait_for, watch_signals

SERVICE = "org.devl0rd.ProcessMonitor.FrameTelemetry"


def telemetry():
    return dbus.Interface(dbus.SessionBus().get_object(SERVICE, "/FrameTelemetry"), SERVICE)


def frames():
    return json.loads(str(telemetry().Frames()))


def watch_error(argument):
    try:
        telemetry().Watch(argument)
    except dbus.DBusException as error:
        return error.get_dbus_name()
    return None


def signalled(lines, pid):
    return any(line.startswith("int64") and line.split()[-1] == str(pid) for line in lines)


def frame_rates(checks):
    subprocess.Popen(["qml6", str(CLIENTS / "animated-client.qml"), "--", "Spinner"])
    checks.expect(wait_for(lambda: "Spinner" in managed_titles(), 60), "the animated client opened")
    pid = next(window["pid"] for window in konveyor_windows() if window["title"] == "Spinner")
    entry = wait_for(lambda: next((frame for frame in frames() if frame["pid"] == pid), None), 60)
    checks.expect(entry is not None, f"Frames reports the animated client ({frames()})")
    if entry:
        checks.expect(entry["fps"] > 0 and entry["frametime"] > 0 and 0 < entry["fps_low"] <= entry["fps"], f"the frame rate is plausible ({entry})")
    lines = watch_signals(f"type='signal',interface='{SERVICE}',member='Frame'", lambda: telemetry().Watch(json.dumps([pid])),
                          lambda seen: signalled(seen, pid))
    checks.expect(signalled(lines, pid), "Watch starts Frame signals for the process")


def bad_requests(checks):
    for argument in ("not json", "{}", "[0]", '["12"]', "[-4]", "[1.5]"):
        checks.equal(watch_error(argument), "org.freedesktop.DBus.Error.InvalidArgs", f"Watch({argument!r}) is refused")
    checks.equal(watch_error("[]"), None, "Watch([]) stops watching")


def main():
    Checks().run(frame_rates, bad_requests)


if __name__ == "__main__":
    main()
