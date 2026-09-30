import json
import os
import select
import subprocess
import tempfile
import time
from pathlib import Path

CLIENTS = Path(__file__).resolve().parent.parent / "clients"


def qdbus(service, path, method, *args):
    return subprocess.run(["qdbus6", service, path, method, *args], capture_output=True, text=True, check=True).stdout.strip()


def wait_for(condition, timeout=30, interval=0.1):
    deadline = time.monotonic() + timeout
    while True:
        value = condition()
        if value or time.monotonic() >= deadline:
            return value
        time.sleep(interval)


def marked_lines(marker):
    with open(os.environ["KONVEYOR_KWIN_LOG"], errors="replace") as log:
        return [line.strip().split("|", 1)[1] for line in log if marker in line and "|" in line]


def run_script(body, seconds=None):
    marker = f"PROBE{time.monotonic_ns()}"
    end = f"{marker}|__end__"
    with tempfile.NamedTemporaryFile("w", suffix=".js", delete=False) as script:
        script.write(body.replace("MARK", marker) + f'\nprint("{end}");\n')
        path = script.name
    script_id = qdbus("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting.loadScript", path, marker)
    qdbus("org.kde.KWin", f"/Scripting/Script{script_id}", "org.kde.kwin.Script.run")
    finished = wait_for(lambda: "__end__" in marked_lines(marker), 60)
    if seconds is not None:
        time.sleep(seconds)
    qdbus("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting.unloadScript", marker)
    os.unlink(path)
    if not finished:
        raise RuntimeError(f"the KWin script {marker} never finished")
    return [line for line in marked_lines(marker) if line != "__end__"]


def for_window(title, statement):
    return f'for (const w of workspace.windowList()) {{ if (!w.deleted && w.caption == "{title}") {{ {statement} }} }}'


def activate(title):
    run_script(for_window(title, "workspace.activeWindow = w;"))
    return wait_for(lambda: active_title() == title)


def active_title():
    printed = run_script('const a = workspace.activeWindow; print("MARK|" + (a ? a.caption : "none"));')
    return printed[0] if printed else None


def window_state(title):
    printed = run_script(for_window(title, 'const g = w.frameGeometry; print("MARK|" + w.fullScreen + "|" + g.x + "," + g.y + " " + g.width + "x" + g.height);'))
    return printed[0] if printed else None


def window_minimized(title):
    printed = run_script(for_window(title, 'print("MARK|" + w.minimized);'))
    return printed[0] == "true" if printed else None


def watch_changes(title, seconds):
    return run_script(for_window(title, 'w.frameGeometryChanged.connect(() => print("MARK|geometry|" + w.frameGeometry));'
                                        ' w.fullScreenChanged.connect(() => print("MARK|fullscreen|" + w.fullScreen));'), seconds)


def frames():
    printed = run_script('for (const w of workspace.windowList()) { if (!w.deleted && w.normalWindow) { const g = w.frameGeometry; '
                         'print("MARK|" + w.caption + "|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height); } }')
    result = {}
    for line in printed:
        title, *values = line.split("|")
        result[title] = tuple(float(value) for value in values)
    return result


def frame(title):
    return frames().get(title)


def intersects(a, b):
    return a[0] < b[0] + b[2] and b[0] < a[0] + a[2] and a[1] < b[1] + b[3] and b[1] < a[1] + a[3]

def kwin_titles():
    return run_script('for (const w of workspace.windowList()) { if (!w.deleted) print("MARK|" + w.caption); }')


def konveyor(method, *args):
    return qdbus("org.kde.Konveyor", "/Konveyor", f"org.kde.Konveyor.{method}", *args)


def konveyor_windows():
    return json.loads(konveyor("Windows"))


def managed_titles():
    return sorted(window["title"] for window in konveyor_windows())


def managed_now():
    try:
        return set(managed_titles())
    except subprocess.CalledProcessError:
        return set()


def reload_konveyor(titles):
    qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.unloadEffect", "konveyor_effect")
    loaded = qdbus("org.kde.KWin", "/Effects", "org.kde.kwin.Effects.loadEffect", "konveyor_effect") == "true"
    return loaded and wait_for(lambda: set(titles) <= managed_now(), 60)


def konveyor_action(name, *arguments):
    return konveyor("Action", json.dumps({"name": name, "arguments": list(arguments), "properties": {}}))


def open_client(title, width=700, height=500, client="client.qml", managed=True):
    log = open(os.environ["KONVEYOR_KWIN_LOG"], "a")
    process = subprocess.Popen(["qml6", str(CLIENTS / client), "--", title, str(width), str(height)], stdout=log, stderr=subprocess.STDOUT)
    appeared = wait_for(lambda: title in (managed_titles() if managed else kwin_titles()), 60)
    if not appeared:
        raise RuntimeError(f"the {title} window never appeared")
    return process



def read_lines(stream, lines, done, timeout):
    deadline = time.monotonic() + timeout
    while not done(lines):
        ready, _, _ = select.select([stream], [], [], max(0.0, deadline - time.monotonic()))
        line = stream.readline().decode(errors="replace") if ready else ""
        if not line:
            return False
        lines.append(line.strip())
    return True


def watch_signals(match, action, done, timeout=30):
    monitor = subprocess.Popen(["dbus-monitor", "--monitor", match], stdout=subprocess.PIPE, bufsize=0)
    lines = []
    try:
        if not read_lines(monitor.stdout, lines, lambda seen: any("NameLost" in line for line in seen), timeout):
            raise RuntimeError("dbus-monitor did not start")
        lines.clear()
        action()
        read_lines(monitor.stdout, lines, done, timeout)
    finally:
        monitor.terminate()
        monitor.wait(timeout=10)
    return lines
