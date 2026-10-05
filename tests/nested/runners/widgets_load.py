#!/usr/bin/env python3
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from checks import Checks
from kwinsession import wait_for

REPO = Path(__file__).resolve().parents[3]
ROOT = Path(os.environ["KONVEYOR_TEST_ROOT"])
WIDGETS = ROOT / "widgets"
LOG = ROOT / "plasmashell.log"
LOAD_ERRORS = ("error when loading applet", "Could not create attached properties object", "TypeError", "ReferenceError", "Required property")


def plasma(script):
    return subprocess.run(["qdbus6", "org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell.evaluateScript", script],
                          capture_output=True, text=True).stdout.strip()


def install(checks):
    shutil.copytree(REPO / "widgets", WIDGETS, symlinks=True)
    script = 'say() { echo "$*"; }; source "$WIDGETS_DIR/lib.sh"; install_plasmoids'
    result = subprocess.run(["bash", "-c", script], env={**os.environ, "WIDGETS_DIR": str(WIDGETS)}, capture_output=True, text=True)
    checks.equal(result.returncode, 0, f"the widgets install into the test home ({result.stderr.strip()[-400:]})")


def widget_ids():
    return sorted(json.loads(path.read_text())["KPlugin"]["Id"] for path in WIDGETS.glob("*/plasmoids/*/metadata.json"))


def load_errors(widget):
    return [line for line in LOG.read_text(errors="replace").splitlines() if widget in line and any(error in line for error in LOAD_ERRORS)]


def every_widget_loads(checks):
    checks.expect(wait_for(lambda: plasma("print(desktops().length);") not in ("", "0"), 180, 1), "plasmashell is up with a desktop")
    ids = widget_ids()
    checks.expect(len(ids) > 10, f"every widget was found ({ids})")
    for widget in ids:
        plasma(f'desktops()[0].addWidget("{widget}");')
    time.sleep(5)
    for widget in ids:
        checks.equal(load_errors(widget), [], f"{widget} loads")


def main():
    Checks().run(install, every_widget_loads)


if __name__ == "__main__":
    main()
