#!/usr/bin/env python3
"""Pick the first starting point in Appearance and see the settings write it as the focus ring style and the focused window's frame start moving."""
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import link_konveyor_imports, run_runner

    with tempfile.TemporaryDirectory(prefix="konveyor-appearance-imports-") as imports:
        setup = f'export QML_IMPORT_PATH="{link_konveyor_imports(imports)}"\n'
        return run_runner(HERE / "runners" / "appearance_first_use.py", timeout=240, clients=("A", "B", "C"), setup=setup)


if __name__ == "__main__":
    sys.exit(main())
