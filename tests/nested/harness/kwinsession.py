import json
import os
import subprocess
import tempfile
import time


def qdbus(service, path, method, *args):
    return subprocess.run(["qdbus6", service, path, method, *args], capture_output=True, text=True, check=True).stdout.strip()


def run_script(body, seconds=0.3):
    marker = f"PROBE{time.monotonic_ns()}"
    with tempfile.NamedTemporaryFile("w", suffix=".js", delete=False) as script:
        script.write(body.replace("MARK", marker))
        path = script.name
    script_id = qdbus("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting.loadScript", path, marker)
    qdbus("org.kde.KWin", f"/Scripting/Script{script_id}", "org.kde.kwin.Script.run")
    time.sleep(seconds)
    qdbus("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting.unloadScript", marker)
    os.unlink(path)
    with open(os.environ["KONVEYOR_KWIN_LOG"], errors="replace") as log:
        return [line.strip().split("|", 1)[1] for line in log if marker in line]


def for_window(title, statement):
    return f'for (const w of workspace.windowList()) {{ if (!w.deleted && w.caption == "{title}") {{ {statement} }} }}'


def activate(title):
    run_script(for_window(title, "workspace.activeWindow = w;"))


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


def konveyor_windows():
    return json.loads(qdbus("org.kde.Konveyor", "/Konveyor", "org.kde.Konveyor.Windows"))


def konveyor_action(name, *arguments):
    qdbus("org.kde.Konveyor", "/Konveyor", "org.kde.Konveyor.Action", json.dumps({"name": name, "arguments": list(arguments), "properties": {}}))
