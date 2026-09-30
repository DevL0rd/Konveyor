import importlib.machinery
import importlib.util
import os
from pathlib import Path
from unittest import mock


REPO = Path(__file__).resolve().parents[3]
SCRIPT = REPO / "src" / "cheatsheet" / "konveyor-cheatsheet.in"


def load_module(root):
    root = Path(root)
    environment = {
        "HOME": str(root / "home"),
        "XDG_CONFIG_HOME": str(root / "config"),
        "XDG_DATA_HOME": str(root / "data"),
        "XDG_RUNTIME_DIR": str(root / "runtime"),
        "KONVEYOR_CHEATSHEET_QML": str(root / "Cheatsheet.qml"),
        "KONVEYOR_CHEATSHEET_VIEWER": str(root / "viewer"),
        "DBUS_SESSION_BUS_ADDRESS": "unix:path=/nonexistent/konveyor-test-bus",
    }
    for name in ("home", "config", "data", "runtime"):
        (root / name).mkdir(exist_ok=True)
    with mock.patch.dict(os.environ, environment):
        loader = importlib.machinery.SourceFileLoader("konveyor_cheatsheet", str(SCRIPT))
        module = importlib.util.module_from_spec(importlib.util.spec_from_loader(loader.name, loader))
        loader.exec_module(module)
    return module, environment
