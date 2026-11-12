#!/usr/bin/env python3
"""Keep a key the user gave to another component's action when Konveyor loads again."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

OTHER_DEFAULT_HOLDER = """[Desktop Entry]
Type=Application
Name=Konveyor Test Other
Exec=true
Actions=RecordRegion;

[Desktop Action RecordRegion]
Name=Record Region
Exec=true
X-KDE-Shortcuts=Meta+R
"""


def main():
    from nested import run_runner

    return run_runner(HERE / "runners" / "user_shortcut_keys.py", clients=(), timeout=240, global_shortcuts=True,
                      files={"data/kglobalaccel/konveyor-test-other.desktop": OTHER_DEFAULT_HOLDER})


if __name__ == "__main__":
    sys.exit(main())
