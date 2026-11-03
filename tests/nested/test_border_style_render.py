#!/usr/bin/env python3
"""Render every border layer kind through the settings preview item on a real scene graph and read the frame's pixels."""

import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SCRIPT = """
export QT_QPA_PLATFORM=wayland
export QML_IMPORT_PATH="{imports}"
if "{binary}" > "$KONVEYOR_REPORT" 2>&1; then echo "RESULT: PASS" >> "$KONVEYOR_REPORT"; fi
"""


def main():
    from nested import build_dir, link_konveyor_imports, run_script

    with tempfile.TemporaryDirectory(
        prefix="konveyor-border-style-imports-"
    ) as imports:
        script = SCRIPT.format(
            imports=link_konveyor_imports(imports),
            binary=build_dir() / "bin" / "border_style_render",
        )
        return run_script(script, timeout=240)


if __name__ == "__main__":
    sys.exit(main())
