#!/usr/bin/env python3
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

SLOW_SCALE = """
[Effect-scale]
Duration=8000
"""


def main():
    from nested import run_runner
    from test_kontrol_panel import SETUP, stage

    with tempfile.TemporaryDirectory(prefix="konveyor-kontrol-panel-") as temporary:
        panel, imports, _ = stage(Path(temporary))
        return run_runner(HERE / "runners" / "kontrol_panel_backdrop.py", clients=(), timeout=420, setup=SETUP.format(imports=imports, panel=panel),
                          extra_kwinrc=SLOW_SCALE)


if __name__ == "__main__":
    sys.exit(main())
