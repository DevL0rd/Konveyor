#!/usr/bin/env python3
import shutil
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SETUP = """export QML_IMPORT_PATH="{imports}"
export KONVEYOR_KONTROL_PANEL_DIR="{panel}"
kbuildsycoca6 >/dev/null 2>&1
"""

PROGRAM_DESKTOP = """[Desktop Entry]
Type=Application
Name=Kontrol Panel service
Exec={program} {panel}
NoDisplay=true
X-KDE-Wayland-Interfaces=org_kde_plasma_window_management
"""

PROBE_DESKTOP = """[Desktop Entry]
Type=Application
Name=Konveyor Probe Editor
Exec=true
NoDisplay=true
"""


def stage(root):
    from nested import REPO, build_dir

    panel = root / "kontrol-panel"
    shutil.copytree(REPO / "widgets" / "portals" / "kontrol-panel", panel)
    shutil.copytree(REPO / "widgets" / "portals" / "shared" / "launcher", panel, dirs_exist_ok=True)
    lib = panel / "lib"
    lib.mkdir()
    for pattern in ("*.qml", "*.js"):
        for path in (REPO / "widgets" / "shared" / "common").glob(pattern):
            shutil.copy(path, lib)
    shutil.copy(REPO / "widgets" / "shared" / "MonitorOverlay.qml", lib)
    shutil.copytree(REPO / "widgets" / "portals" / "shared" / "lib", lib, dirs_exist_ok=True)
    konveyor = root / "imports" / "org" / "kde" / "konveyor"
    konveyor.mkdir(parents=True)
    (konveyor / "settings").symlink_to(build_dir() / "bin" / "org" / "kde" / "konveyor" / "settings")
    (konveyor / "components").symlink_to(REPO / "src" / "components")
    return panel, root / "imports", build_dir() / "bin" / "konveyor-kontrol-panel"


def main():
    from nested import run_runner

    with tempfile.TemporaryDirectory(prefix="konveyor-kontrol-panel-") as temporary:
        panel, imports, program = stage(Path(temporary))
        files = {"data/applications/konveyor-kontrol-panel.desktop": PROGRAM_DESKTOP.format(program=program, panel=panel),
                 "data/applications/org.qt-project.qml.desktop": PROBE_DESKTOP}
        return run_runner(HERE / "runners" / "kontrol_panel.py", clients=(), timeout=300, setup=SETUP.format(imports=imports, panel=panel),
                          files=files, permission_checks=True)


if __name__ == "__main__":
    sys.exit(main())
