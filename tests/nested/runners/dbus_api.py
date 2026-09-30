#!/usr/bin/env python3
import json
import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks, cli, config_path, default_config, notifications
from fakepointer import touch
from kwinsession import activate, for_window, konveyor, konveyor_action, qdbus, run_script, wait_for, watch_signals
from nested import REPO

ROOT = Path(os.environ["KONVEYOR_TEST_ROOT"])
FAILED = "Konveyor: failed to load config"


def bind_keys():
    return sorted(bind["key"] for bind in json.loads(konveyor("Binds")))


def spawn_binds(*keys):
    return "binds {\n" + "".join(f'    {key} {{ spawn "true"; }}\n' for key in keys) + "}\n"


def failures():
    return [entry for entry in notifications() if entry["summary"] == FAILED]


def window_center(title):
    frame = run_script(for_window(title, 'const g = w.frameGeometry; print("MARK|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height);'))
    x, y, width, height = (float(value) for value in frame[0].split("|"))
    return round(x + width / 2), round(y + height / 2)


def version(checks):
    expected = re.search(r"project\(konveyor VERSION ([0-9.]+)", (REPO / "CMakeLists.txt").read_text()).group(1)
    checks.equal(json.loads(konveyor("Version")), {"version": expected}, "Version")
    checks.equal(cli("msg", "version").stdout.strip(), expected, "konveyor msg version")


def queries(checks):
    windows = json.loads(konveyor("Windows"))
    checks.equal(sorted(window["title"] for window in windows), ["A", "B", "C"], "Windows lists the three clients")
    workspaces = json.loads(konveyor("Workspaces"))
    checks.expect(any(workspace["is_focused"] for workspace in workspaces), f"Workspaces has a focused workspace ({workspaces})")
    screens = run_script('for (const o of workspace.screens) print("MARK|" + o.name);')
    checks.equal([output["name"] for output in json.loads(konveyor("Outputs"))], screens, "Outputs matches KWin's screens")
    checks.equal([output["name"] for output in json.loads(konveyor("FocusedOutput"))], screens, "FocusedOutput")
    checks.expect(len(bind_keys()) > 50, "Binds lists the default binds")
    gestures = json.loads(konveyor("Gestures"))
    checks.expect({"device": "touchpad", "motion": "swipe-horizontal"}.items() <= next(
        (gesture for gesture in gestures if gesture["motion"] == "swipe-horizontal"), {}).items(), f"Gestures has the touchpad swipe ({gestures})")
    checks.equal(json.loads(konveyor("OverviewState")), {"is_open": False}, "OverviewState")
    checks.equal(json.loads(konveyor("LastBind")), {"count": 0, "key": "", "action": ""}, "LastBind before any bind")
    checks.equal(konveyor("MultiTouchActive"), "false", "MultiTouchActive")
    for request in ("windows", "workspaces", "outputs", "focused-output", "binds"):
        result = cli("msg", "--json", request)
        checks.expect(result.returncode == 0 and json.loads(result.stdout) is not None, f"konveyor msg --json {request} prints JSON")
        checks.expect(cli("msg", request).returncode == 0, f"konveyor msg {request} succeeds")


def focused_window(checks):
    activate("B")
    checks.expect(wait_for(lambda: json.loads(konveyor("FocusedWindow") or "null") and json.loads(konveyor("FocusedWindow"))["title"] == "B"),
                  "FocusedWindow follows the active window")
    checks.equal(konveyor_action("focus-workspace", "2"), "", "focus-workspace 2")
    checks.expect(wait_for(lambda: konveyor("FocusedWindow") == "null"), f"FocusedWindow on an empty workspace is null ({konveyor('FocusedWindow')!r})")
    result = cli("msg", "focused-window")
    checks.expect(result.returncode == 0 and "No window is focused." in result.stdout, f"konveyor msg focused-window with nothing focused ({result.stdout!r})")
    checks.equal(cli("msg", "--json", "focused-window").stdout.strip(), "null", "konveyor msg --json focused-window with nothing focused")
    checks.equal(konveyor_action("focus-workspace", "1"), "", "focus-workspace 1")


def actions(checks):
    error = konveyor("Action", "{not json")
    checks.expect("JSON" in error, f"Action with bad JSON explains the JSON error ({error!r})")
    checks.expect(konveyor("Action", "[]") != "", "Action with a JSON array fails")
    checks.expect(konveyor("Action", "{}") != "", "Action without a name fails")
    checks.expect(konveyor_action("no-such-action") != "", "Action with an unknown action fails")
    checks.expect(konveyor("Action", json.dumps({"name": "close-window", "arguments": [], "properties": {}, "id": 99999})) != "",
                  "Action targeting a window that does not exist fails")
    checks.equal(konveyor_action("focus-column-left"), "", "a valid Action")
    checks.equal(cli("msg", "action", "focus-column-right").returncode, 0, "konveyor msg action")
    checks.equal(cli("msg", "action", "no-such-action").returncode, 1, "konveyor msg action with an unknown action")
    checks.equal(cli("msg", "no-such-request").returncode, 1, "konveyor msg with an unknown request")


def load_config_file(checks):
    other = ROOT / "other" / "config.kdl"
    other.parent.mkdir()
    other.write_text(spawn_binds("Mod+F1"))
    checks.equal(konveyor("LoadConfigFile", str(other)), "", "LoadConfigFile of another file")
    checks.equal(bind_keys(), ["Super+F1"], "the other file's binds are loaded")
    other.write_text(spawn_binds("Mod+F1", "Mod+F2"))
    checks.expect(wait_for(lambda: bind_keys() == ["Super+F1", "Super+F2"]), f"editing the loaded file reloads it ({bind_keys()})")
    config_path().write_text(default_config() + "\n// touched\n")
    other.write_text(spawn_binds("Mod+F1", "Mod+F2", "Mod+F3"))
    checks.expect(wait_for(lambda: len(bind_keys()) == 3), f"editing the main config does not switch back to it ({bind_keys()})")
    checks.equal(konveyor("LoadConfigFile", ""), "", "LoadConfigFile with no path")
    checks.equal(len(bind_keys()), 3, "LoadConfigFile with no path reloads the file in use")
    before = len(failures())
    missing = konveyor("LoadConfigFile", str(ROOT / "missing.kdl"))
    checks.expect("missing.kdl" in missing, f"LoadConfigFile of a missing file reports it ({missing!r})")
    broken = ROOT / "broken.kdl"
    broken.write_text("binds {\n")
    checks.expect(konveyor("LoadConfigFile", str(broken)) != "", "LoadConfigFile of invalid KDL fails")
    checks.equal(len(bind_keys()), 3, "failed loads keep the last good config")
    checks.expect(wait_for(lambda: len(failures()) == before + 2), f"each failed load shows a notification ({failures()})")
    other.write_text(spawn_binds("Mod+F1", "Mod+F2", "Mod+F3", "Mod+F4"))
    checks.expect(wait_for(lambda: len(bind_keys()) == 4), "the file in use is still followed after failed loads")
    checks.equal(cli("msg", "load-config-file", str(config_path())).returncode, 0, "konveyor msg load-config-file back to the main config")
    checks.expect(len(bind_keys()) > 50, "the main config is in use again")
    checks.equal(cli("msg", "load-config-file", str(broken)).returncode, 1, "konveyor msg load-config-file with invalid KDL")


def reload_invalid(checks):
    silent = "config-notification { disable-failed; }\n"
    config_path().write_text(spawn_binds("Mod+F8"))
    checks.expect(wait_for(lambda: bind_keys() == ["Super+F8"]), "editing the main config reloads it")
    before = len(failures())
    config_path().write_text(spawn_binds("Mod+F8") + "binds {\n")
    checks.expect(wait_for(lambda: len(failures()) == before + 1), "an invalid config shows a notification")
    body = failures()[-1]["body"] if failures() else ""
    checks.expect("config.kdl" in body, f"the notification names the file ({body!r})")
    checks.equal(bind_keys(), ["Super+F8"], "an invalid config keeps the last good config")
    config_path().write_text(silent + spawn_binds("Mod+F9"))
    checks.expect(wait_for(lambda: bind_keys() == ["Super+F9"]), "fixing the config reloads it")
    config_path().write_text(silent + "binds {\n")
    checks.expect(konveyor("LoadConfigFile", "") != "", "the broken config fails to load")
    config_path().write_text(spawn_binds("Mod+F10"))
    checks.expect(wait_for(lambda: bind_keys() == ["Super+F10"]), "the config loads again")
    config_path().write_text("binds {\n")
    checks.expect(wait_for(lambda: len(failures()) == before + 2), f"disable-failed silenced exactly one failure ({failures()[before:]})")
    config_path().write_text(default_config())
    checks.expect(wait_for(lambda: len(bind_keys()) > 50), "the default config is back")


def multitouch(checks):
    values = []

    def done(lines):
        values[:] = [line.split()[-1] for line in lines if line.startswith("boolean")]
        return values[-2:] == ["true", "false"]

    watch_signals("type='signal',interface='org.kde.Konveyor',member='MultiTouchChanged'",
                  lambda: touch(2, *window_center("B"), hold_ms=200, steps=4, settle=False), done)
    checks.equal(values, ["true", "false"], "MultiTouchChanged fires for a two-finger touch and its release")
    checks.equal(konveyor("MultiTouchActive"), "false", "MultiTouchActive after the touch")


def answers():
    return subprocess.run(["qdbus6", "org.kde.Konveyor", "/Konveyor", "org.kde.Konveyor.Version"], capture_output=True).returncode == 0


def versioned_reinstall(checks):
    effects = ("org.kde.KWin", "/Effects")
    checks.equal(qdbus(*effects, "org.kde.kwin.Effects.loadEffect", "konveyor_effect_2"), "true", "a second copy of the effect loads")
    checks.expect(answers(), "org.kde.Konveyor still answers")
    qdbus(*effects, "org.kde.kwin.Effects.unloadEffect", "konveyor_effect")
    checks.equal(qdbus(*effects, "org.kde.kwin.Effects.isEffectLoaded", "konveyor_effect"), "false", "the first copy unloads")
    checks.expect(wait_for(answers), "the second copy takes over org.kde.Konveyor")
    checks.equal(sorted(window["title"] for window in json.loads(konveyor("Windows"))), ["A", "B", "C"], "and manages the windows")


def main():
    Checks().run(version, queries, focused_window, actions, load_config_file, reload_invalid, multitouch, versioned_reinstall)


if __name__ == "__main__":
    main()
