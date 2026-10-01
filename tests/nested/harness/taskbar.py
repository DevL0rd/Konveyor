import json
import shutil
from pathlib import Path

from nested import REPO


class TaskbarObservation:
    def __init__(self, root, data_home):
        self.log = root / "widget.log"
        self.target = Path(data_home) / "plasma" / "plasmoids" / "org.devl0rd.konveyor.taskbar"

    def stage(self):
        shutil.copytree(REPO / "widgets" / "taskbar" / "plasmoid", self.target)
        source = self.target / "contents" / "ui" / "Taskbar.qml"
        text = source.read_text()
        observation = '''
    Timer {
        property string previous: ""
        interval: 100
        running: true
        repeat: true
        onTriggered: {
            const state = JSON.stringify({grouped: root.grouped, pins: root.pinnedLaunchers, launchers: root.tasksModel.launcherList,
                entries: root.entries.map(row => {
                    const entry = row.entry || row
                    return {title: entry.title, app: entry.appId, ids: entry.windowIds, group: entry.group, active: entry.active}
                })})
            if (state !== previous) {
                console.log("KONVEYOR_TASKBAR_STATE " + state)
                previous = state
            }
        }
    }
'''
        source.write_text(text[:text.rfind("}")] + observation + "}\n")

    def state(self):
        if not self.log.exists():
            return {}
        for line in reversed(self.log.read_text(errors="replace").splitlines()):
            if "KONVEYOR_TASKBAR_STATE " in line:
                return json.loads(line.split("KONVEYOR_TASKBAR_STATE ", 1)[1])
        return {}

    def titles(self):
        return [entry["title"] for entry in self.state().get("entries", [])]


def clean_snapshot(path, windows):
    import time

    from kwinsession import frames
    from screenshot import capture_workspace

    deadline = time.monotonic() + 12
    missing = []
    while True:
        time.sleep(0.2)
        image = capture_workspace(path)
        geometry = frames()
        missing = []
        for window in windows:
            rect = geometry.get(window["title"])
            if not rect:
                continue
            cx, cy = rect[0] + rect[2] / 2, rect[1] + rect[3] / 2
            if cx < 100 or cy < 50 or cx > image.width - 100 or cy > image.height - 50:
                continue
            region = image.crop((int(cx - 100), int(cy - 40), int(cx + 100), int(cy + 40)))
            painted = sum(1 for red, green, blue, *_ in region.getdata() if min(red, green, blue) > 180)
            if painted < 30:
                missing.append(window["title"])
        if not missing:
            return
        if time.monotonic() >= deadline:
            raise RuntimeError(f"visible synthetic windows have blank rendered regions: {missing}")
