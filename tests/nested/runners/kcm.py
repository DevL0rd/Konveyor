#!/usr/bin/env python3
import re
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from kwinsession import run_script

REPO = Path(__file__).resolve().parents[3]
OURS = re.compile(r"kcm_konveyor|org/kde/konveyor|Konveyor settings page")


def pages():
    return re.findall(r'id: "([a-z]+)"', (REPO / "src" / "settings" / "qml" / "catalog" / "Pages.js").read_text())


def module_window():
    printed = run_script('for (const w of workspace.windowList()) { if (w.resourceClass == "kcm_konveyor") print("MARK|" + w.caption); }')
    return printed[0] if printed else None


def open_page(page, problems):
    with tempfile.TemporaryFile("w+") as log:
        process = subprocess.Popen(["kcmshell6", "kcm_konveyor", "--args", page], stdout=log, stderr=subprocess.STDOUT)
        deadline = time.monotonic() + 20
        caption = None
        while caption is None and time.monotonic() < deadline and process.poll() is None:
            time.sleep(0.5)
            caption = module_window()
        time.sleep(2)
        process.terminate()
        process.wait(timeout=10)
        log.seek(0)
        errors = [line.strip() for line in log if OURS.search(line)]
    print(f"{page}: window {caption!r}, {len(errors)} Konveyor QML messages")
    if caption is None:
        problems.append(f"{page}: the System Settings module never opened a window")
    for error in errors:
        problems.append(f"{page}: {error}")


def main():
    problems = []
    found = pages()
    if len(found) < 10:
        problems.append(f"expected the settings pages from Pages.js, found {found}")
    for page in found:
        open_page(page, problems)
    for problem in problems:
        print("  PROBLEM " + problem)
    print("RESULT:", "FAIL" if problems else "PASS")


if __name__ == "__main__":
    main()
