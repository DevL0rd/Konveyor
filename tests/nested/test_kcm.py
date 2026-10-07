#!/usr/bin/env python3
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SCRIPT = """
export QT_QPA_PLATFORM=wayland
export QML_IMPORT_PATH="{imports}"
python3 {runner} > "$KONVEYOR_REPORT" 2>&1
"""


def main():
    from nested import link_konveyor_imports, run_script

    with tempfile.TemporaryDirectory(prefix="konveyor-kcm-imports-") as imports:
        return run_script(SCRIPT.format(imports=link_konveyor_imports(imports), runner=HERE / "runners" / "kcm.py"), timeout=540)


if __name__ == "__main__":
    sys.exit(main())
