import json
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from fakepointer import Held, click
from kwinsession import activate, for_window, konveyor, konveyor_windows, run_script, wait_for, window_minimized
from nested import build_dir
from taskbar import TaskbarObservation, clean_snapshot

ROOT = Path(os.environ["KONVEYOR_TEST_ROOT"])
CLIENTS = {}
WIDGET = None
OBSERVATION = TaskbarObservation(ROOT, os.environ["XDG_DATA_HOME"])


def columns():
    windows = [window for window in konveyor_windows() if window["title"].startswith("App ")]
    return [window["title"] for window in sorted(windows, key=lambda item: item["layout"]["pos_in_scrolling_layout"])]


def widget_frame():
    lines = run_script('for (const w of workspace.windowList()) { if (!w.deleted && w.resourceClass == "org.kde.plasmawindowed") '
                       '{ const g = w.frameGeometry; print("MARK|" + [g.x, g.y, g.width, g.height].join(",")); } }')
    return tuple(float(value) for value in lines[0].split(",")) if lines else None


def point(index):
    frame = widget_frame()
    return frame[0] + 24 + 48 * index, frame[1] + 24


def icons_follow_columns():
    windows = {str(window["task_id"]).strip("{}").lower(): window for window in konveyor_windows() if window["taskbar_eligible"]}
    ranks = []
    for entry in OBSERVATION.state().get("entries", []):
        positions = [windows[str(task).strip("{}").lower()]["layout"]["pos_in_scrolling_layout"][0]
                     for task in entry["ids"] if str(task).strip("{}").lower() in windows]
        if positions:
            ranks.append(min(positions))
    return ranks == sorted(ranks)


def snapshot(name):
    if not wait_for(icons_follow_columns):
        raise RuntimeError("taskbar icons did not settle to the completed column order")
    active = next(item["id"] for item in json.loads(konveyor("Workspaces")) if item["output"] == "Virtual-0" and item["is_active"])
    windows = [window for window in konveyor_windows() if window["title"].startswith("App ") and window["workspace_id"] == active]
    clean_snapshot(ROOT / f"taskbar-{name}.png", windows)


def open_app(letter, suffix=""):
    title = f"App {letter.upper()}{suffix}"
    with Path(os.environ["KONVEYOR_KWIN_LOG"]).open("a") as log:
        process = subprocess.Popen([str(build_dir() / "bin" / "taskbar_client"), f"konveyor-taskbar-test-{letter}", title, "480", "320"],
                                   stdout=log, stderr=subprocess.STDOUT,
                                   env={**os.environ, "QT_QPA_PLATFORM": os.environ.get("KONVEYOR_TASKBAR_CLIENT_PLATFORM", "wayland")})
    CLIENTS[title] = process
    if not wait_for(lambda: title in [window["title"] for window in konveyor_windows()]):
        raise RuntimeError(f"{title} did not open")
    return process


def start_widget():
    global WIDGET
    with (ROOT / "widget.log").open("a") as log:
        WIDGET = subprocess.Popen(["plasmawindowed", "org.devl0rd.konveyor.taskbar"], stdout=log, stderr=subprocess.STDOUT)
    return wait_for(widget_frame)


def stop_widget():
    run_script('for (const w of workspace.windowList()) { if (w.resourceClass == "org.kde.plasmawindowed") w.closeWindow(); }')
    WIDGET.wait(timeout=15)


def task_click(index, button):
    x, y = point(index)
    with Held() as pointer:
        pointer.send(f"move:{x}:{y}")
        time.sleep(0.1)
        pointer.send(f"button:{button}:1")
        time.sleep(0.05)
        pointer.send(f"button:{button}:0")
        time.sleep(0.05)


def context_action(index, action):
    x, _ = point(index)
    task_click(index, 273)
    time.sleep(0.2)
    click(x + 80, widget_frame()[1] + 20 + 30 * action)
    time.sleep(0.3)


def drag(source, target):
    x, y = point(source)
    tx, ty = point(target)
    with Held() as pointer:
        pointer.send(f"move:{x}:{y}")
        time.sleep(0.1)
        pointer.send("button:272:1")
        time.sleep(0.1)
        for step in range(1, 9):
            pointer.send(f"move:{x + (tx - x) * step / 8}:{y + (ty - y) * step / 8}")
            time.sleep(0.03)
        pointer.send("button:272:0")


def configuration():
    return "\n".join(path.read_text(errors="replace") for path in Path(os.environ["XDG_CONFIG_HOME"]).glob("plasmawindowed*rc") if path.is_file())


def setup(checks):
    OBSERVATION.stage()
    apps = Path(os.environ["XDG_DATA_HOME"]) / "applications"
    apps.mkdir(parents=True, exist_ok=True)
    for letter, icon in [("a", "folder"), ("b", "accessories-text-editor"), ("c", "utilities-terminal")]:
        (apps / f"konveyor-taskbar-test-{letter}.desktop").write_text(
            f'[Desktop Entry]\nType=Application\nName=App {letter.upper()}\nIcon={icon}\n'
            f'Exec={build_dir()}/bin/taskbar_client konveyor-taskbar-test-{letter} "App {letter.upper()}" 480 320\n')
    subprocess.run(["kbuildsycoca6", "--noincremental"], capture_output=True, check=True)
    checks.expect(wait_for(lambda: konveyor("Version")), "Konveyor is available in the isolated compositor")
    open_app("a")
    open_app("b")
    open_app("a", "2")
    open_app("c")
    checks.expect(start_widget(), "the actual Konveyor Taskbar package opens without installation")
    time.sleep(2)
    checks.expect(wait_for(lambda: columns() == ["App A", "App B", "App A2", "App C"]), "attaching the widget preserves existing interleaved columns")
    checks.expect(wait_for(lambda: OBSERVATION.titles() == ["App A", "App B", "App A2", "App C"]), "initial icons project the existing separate-window layout")
    snapshot("attached")
    run_script(for_window("App A2", "w.closeWindow();"))
    checks.expect(wait_for(lambda: columns() == ["App A", "App B", "App C"] and OBSERVATION.titles() == columns()), "icons follow the existing layout after closing A2")
    snapshot("initial")


def reorder_and_pin(checks):
    drag(2, 0)
    if not checks.expect(wait_for(lambda: columns() == ["App C", "App A", "App B"]), "dragging C before A reorders the actual columns"):
        snapshot("drag-failed")
        return
    snapshot("dragged")
    for index in range(3):
        context_action(index, 0)
    checks.expect(wait_for(lambda: all(f"konveyor-taskbar-test-{letter}.desktop" in configuration() for letter in "abc")), "all three launcher pins are persisted")
    drag(1, 0)
    drag(2, 1)
    checks.expect(wait_for(lambda: columns() == ["App A", "App B", "App C"]), "the pinned task order is A B C")
    snapshot("pinned")
    expected = "launchers=" + ",".join(f"applications:konveyor-taskbar-test-{letter}.desktop" for letter in "abc")
    checks.expect(wait_for(lambda: expected in configuration().splitlines()), "the dragged A B C launcher order is saved before restart")


def close_and_restart(checks):
    for index, title in [(2, "App C"), (1, "App B"), (0, "App A")]:
        task_click(index, 274)
        if not checks.expect(wait_for(lambda: title not in columns()), f"middle-click closes {title}"):
            return
    snapshot("launchers-only")
    stop_widget()
    checks.expect(start_widget(), "the widget restarts from its saved launcher configuration")
    time.sleep(1)
    task_click(0, 272)
    checks.expect(wait_for(lambda: columns() == ["App A"]), "clicking a saved pinned launcher starts its application")
    task_click(0, 274)
    checks.expect(wait_for(lambda: not columns()), "middle-click closes the application started from its launcher")
    for placement in ["right", "left"]:
        config = ROOT / "config" / "konveyor" / "config.kdl"
        config.write_text(f'animations {{ off; }}\nlayout {{ default-column-width {{ proportion 0.22; }}; new-column-position "{placement}"; group-app-windows "off"; }}\n'
                          'window-rule { match app-id="^org.kde.plasmawindowed$"; open-floating true; }\n')
        checks.equal(konveyor("LoadConfigFile", str(config)), "", f"set {placement} placement")
        for letter in "cab":
            open_app(letter)
        checks.expect(wait_for(lambda: columns() == ["App A", "App B", "App C"]), f"saved A B C pins override C A B launch order and {placement} placement")
        snapshot(f"launch-{placement}")
        if placement == "right":
            for title in ["App A", "App B", "App C"]:
                run_script(for_window(title, "w.closeWindow();"))
            wait_for(lambda: not columns())


def set_grouping(mode, extra=""):
    config = ROOT / "config" / "konveyor" / "config.kdl"
    config.write_text(f'animations {{ off; }}\nlayout {{ default-column-width {{ proportion 0.22; }}; group-app-windows "{mode}"; }}\n'
                      'window-rule { match app-id="^org.kde.plasmawindowed$"; open-floating true; }\n' + extra)
    return konveyor("LoadConfigFile", str(config))


def move_column(title, position):
    activate(title)
    window = next(window for window in konveyor_windows() if window["title"] == title)
    return konveyor("Action", json.dumps({"name": "move-column-to-index", "arguments": [str(position)], "id": window["id"]}))


def clarified_requirements(checks):
    pins = OBSERVATION.state().get("pins")
    saved = [line for line in configuration().splitlines() if line.startswith("launchers=")]
    checks.equal(move_column("App C", 1), "", "move C's column before A")
    time.sleep(1)
    checks.equal(columns(), ["App C", "App A", "App B"], "reverse synchronization does not undo the completed column move")
    checks.expect(wait_for(lambda: OBSERVATION.titles() == ["App C", "App A", "App B"]), "completed column order updates actual taskbar icons")
    checks.equal(OBSERVATION.state().get("pins"), pins, "reverse synchronization preserves saved launcher pins")
    checks.equal([line for line in configuration().splitlines() if line.startswith("launchers=")], saved, "reverse synchronization leaves persisted launcher configuration unchanged")
    snapshot("reverse")
    run_script(for_window("App C", "w.closeWindow();"))
    checks.expect(wait_for(lambda: "App C" not in columns()), "close C after reverse ordering")
    open_app("c")
    checks.expect(wait_for(lambda: columns() == ["App A", "App B", "App C"]), "a new launch respects saved pins after reverse ordering")
    checks.equal(set_grouping("off"), "", "apply effective Konveyor grouping off")
    open_app("a", "2")
    checks.expect(wait_for(lambda: OBSERVATION.state().get("grouped") is False and len(OBSERVATION.titles()) == 4), "Follow Konveyor off shows separate window icons")
    checks.equal(set_grouping("beside"), "", "apply effective Konveyor grouping beside")
    checks.expect(wait_for(lambda: OBSERVATION.state().get("grouped") is True and len(OBSERVATION.titles()) == 3), "Follow Konveyor beside groups application icons")
    checks.equal(move_column("App A2", 3), "", "interleave A columns around B")
    checks.expect(wait_for(lambda: columns() == ["App A", "App B", "App A2", "App C"]), "reverse grouping preserves interleaved columns")
    checks.expect(wait_for(lambda: OBSERVATION.titles() == ["App A", "App B", "App C"]), "a grouped icon follows its first eligible column")
    snapshot("interleaved")
    run_script(for_window("App C", "w.closeWindow();"))
    checks.expect(wait_for(lambda: "App C" not in columns()), "close C beside interleaved grouped columns")
    open_app("c")
    checks.expect(wait_for(lambda: columns() == ["App A", "App B", "App A2", "App C"]), "a launch preserves existing interleaved grouped columns until an icon drag")
    checks.equal(set_grouping("stack"), "", "apply effective Konveyor grouping stack")
    checks.expect(wait_for(lambda: OBSERVATION.state().get("grouped") is True and len(OBSERVATION.titles()) == 3), "Follow Konveyor stack groups application icons")
    checks.equal(set_grouping("beside", 'window-rule { match title="^App A2$"; group-app-windows "off"; }\n'), "", "apply conflicting effective rules for A's windows")
    checks.expect(wait_for(lambda: len(OBSERVATION.titles()) == 4), "conflicting effective grouping rules keep same-app windows separate")
    run_script(for_window("App A2", "w.closeWindow();"))
    checks.expect(wait_for(lambda: "App A2" not in columns()), "close the grouping regression window")
    checks.equal(set_grouping("beside"), "", "restore beside grouping")
    move_column("App A", 1)
    move_column("App B", 2)
    checks.expect(wait_for(lambda: columns() == ["App A", "App B", "App C"]), "restore the ordinary column order")


def minimize_and_group(checks):
    checks.expect(wait_for(lambda: OBSERVATION.titles() == ["App A", "App B", "App C"]), "icons settle after completed column moves")
    activate("App B")
    checks.expect(wait_for(lambda: OBSERVATION.state().get("entries", [{}, {}])[1].get("active")), "B is active in the actual task model")
    task_click(1, 272)
    checks.expect(wait_for(lambda: window_minimized("App B")), "left-click on active B minimizes it")
    snapshot("minimized")
    task_click(1, 272)
    checks.expect(wait_for(lambda: window_minimized("App B") is False), "left-click restores minimized B")
    open_app("a", "2")
    time.sleep(1)
    before = columns()
    app_columns = [title for title in before if title.startswith("App A")]
    checks.equal(len(app_columns), 2, "grouped A owns two separate columns")
    drag(0, 2)
    checks.expect(wait_for(lambda: columns() == ["App B", "App C", *app_columns]), "dragging grouped A moves both columns with their relative order intact")
    snapshot("grouped")
    context_action(2, 1)
    context_action(2, 1)
    checks.expect(wait_for(lambda: "groupingMode=separate" in configuration()), "grouping can be disabled and is persisted")
    checks.expect(wait_for(lambda: columns() == ["App B", "App C", "App A", "App A2"]), "ungrouped window icons order their owning columns")
    snapshot("ungrouped")
    drag(3, 0)
    checks.expect(wait_for(lambda: columns() == ["App A2", "App B", "App C", "App A"]), "dragging ungrouped A2 moves only its owning column")
    snapshot("ungrouped-dragged")
    context_action(0, 0)
    checks.expect(wait_for(lambda: "konveyor-taskbar-test-a.desktop" not in configuration()), "an application can be unpinned without closing its windows")
    checks.equal(len(columns()), 4, "unpinning keeps all running windows")
    context_action(0, 1)
    checks.expect(wait_for(lambda: len(OBSERVATION.titles()) == 3), "grouping can return to Follow Konveyor")
    activate("App A2")
    checks.expect(wait_for(lambda: OBSERVATION.state().get("entries", [{}])[0].get("active")), "A2 is active inside its application group")
    task_click(0, 272)
    checks.expect(wait_for(lambda: window_minimized("App A2")), "left-click on a grouped active task minimizes its active window")
    run_script(for_window("App A2", "w.minimized = false; workspace.activeWindow = w;"))
    checks.expect(wait_for(lambda: window_minimized("App A2") is False), "restore A2 before verifying grouped close")
    activate("App A2")
    checks.expect(wait_for(lambda: OBSERVATION.state().get("entries", [{}])[0].get("active")), "A2 is active before closing its group task")
    task_click(0, 274)
    checks.expect(wait_for(lambda: "App A2" not in columns() and "App A" in columns()), "middle-click on a group closes its active window and preserves its other window")
    snapshot("grouped-close")


def scenario(checks):
    for step in [setup, reorder_and_pin, close_and_restart, clarified_requirements, minimize_and_group]:
        print(f"--- {step.__name__}", flush=True)
        step(checks)
        if checks.problems:
            return


Checks().run(scenario)
