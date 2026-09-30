import os
import sys
from pathlib import Path

if os.environ.get("KONVEYOR_PYCOVERAGE_DIR"):
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    import pycoverage

    sys.path.pop(0)
    pycoverage.start(os.environ["KONVEYOR_PYCOVERAGE_DIR"])
