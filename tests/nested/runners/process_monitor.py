#!/usr/bin/env python3
import json
import subprocess
import sys
import time
from pathlib import Path

import dbus
import dbus.bus

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import CLIENTS, konveyor_windows, managed_titles, run_script, wait_for, watch_signals

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


def spinner_pid():
    return next(window["pid"] for window in konveyor_windows() if window["title"] == "Spinner")


def unwatch_stops_signals(checks):
    pid = spinner_pid()
    telemetry().Watch(json.dumps([pid]))
    telemetry().Watch("[]")
    lines = watch_signals(f"type='signal',interface='{SERVICE}',member='Frame'", lambda: None, lambda seen: signalled(seen, pid), timeout=3)
    checks.expect(not signalled(lines, pid), "after Watch([]) no Frame signals arrive for the animating window")


def service_registered():
    names = subprocess.run(["qdbus6"], capture_output=True, text=True).stdout.split()
    return SERVICE in names


def reload_keeps_reporting(checks):
    effects = ("org.kde.KWin", "/Effects", "org.kde.kwin.Effects")
    subprocess.run(["qdbus6", *effects[:2], f"{effects[2]}.unloadEffect", "process_monitor_telemetry"], check=True)
    checks.expect(wait_for(lambda: not service_registered()), "unloading the telemetry effect releases its D-Bus name")
    subprocess.run(["qdbus6", *effects[:2], f"{effects[2]}.loadEffect", "process_monitor_telemetry"], check=True)
    checks.expect(wait_for(service_registered), "loading it again takes the name back")
    pid = spinner_pid()
    checks.expect(wait_for(lambda: any(frame["pid"] == pid for frame in frames()), 30),
                  f"the reloaded effect reports a window that was already open ({frames()})")


def effect_loaded():
    result = subprocess.run(["qdbus6", "org.kde.KWin", "/Effects", "org.kde.kwin.Effects.isEffectLoaded", "process_monitor_telemetry"],
                            capture_output=True, text=True)
    return result.stdout.strip() == "true"


def effect_call(method):
    subprocess.run(["qdbus6", "org.kde.KWin", "/Effects", f"org.kde.kwin.Effects.{method}", "process_monitor_telemetry"], check=True)


def name_held_elsewhere(checks):
    effect_call("unloadEffect")
    checks.expect(wait_for(lambda: not service_registered()), "unloading the telemetry effect releases its D-Bus name")
    other = dbus.SessionBus(private=True)
    try:
        reply = other.request_name(SERVICE, dbus.bus.NAME_FLAG_DO_NOT_QUEUE)
        checks.equal(reply, dbus.bus.REQUEST_NAME_REPLY_PRIMARY_OWNER, "another process takes the telemetry name")
        effect_call("loadEffect")
        checks.expect(wait_for(effect_loaded), "the effect still loads when its name is taken")
        checks.equal(str(other.get_name_owner(SERVICE)), other.get_unique_name(), "the effect leaves the name with its owner")
        try:
            telemetry().Frames()
            answered = True
        except dbus.DBusException:
            answered = False
        checks.expect(not answered, "the effect does not answer on the object path it could not claim")
        effect_call("unloadEffect")
        checks.expect(wait_for(lambda: not effect_loaded()), "the effect that never held the name unloads")
        checks.equal(str(other.get_name_owner(SERVICE)), other.get_unique_name(), "unloading it leaves the name with its owner")
    finally:
        other.close()
    checks.expect(wait_for(lambda: not service_registered()), "the name is free once its owner is gone")
    effect_call("loadEffect")
    checks.expect(wait_for(service_registered), "the effect takes the name once it is free")
    checks.expect(isinstance(frames(), list), "and answers Frames")


def windows_without_a_process_id(checks):
    reported = []
    for step in range(12):
        offset = 40 * (step % 2)
        run_script(f"workspace.showOutline(100 + {offset}, 100, 300 + {offset}, 200);")
        reported.extend(frames())
        time.sleep(0.1)
    run_script("workspace.hideOutline();")
    checks.expect(all(frame["pid"] > 0 for frame in reported), f"Frames leaves out windows without a process id ({reported})")


def main():
    Checks().run(frame_rates, bad_requests, unwatch_stops_signals, windows_without_a_process_id, reload_keeps_reporting, name_held_elsewhere)


if __name__ == "__main__":
    main()
