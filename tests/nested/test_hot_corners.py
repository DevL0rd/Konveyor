#!/usr/bin/env python3
"""Push the pointer into screen corners on two outputs and check the hot corners, including a per-output override."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))

INSTANT_EDGES = """
[Windows]
ElectricBorderDelay=0
ElectricBorderCooldown=0
ElectricBorderPushbackPixels=0

[Effect-overview]
BorderActivate=9
"""

VIRTUAL_1_OFF = """
output "Virtual-1" {
    hot-corners {
        off
    }
}
"""


def main():
    from nested import REPO, run_runner

    config = (REPO / "data" / "default-config.kdl").read_text()
    corners = config.replace("hot-corners {\n        off\n    }", "hot-corners {\n        top-left\n        top-right\n    }")
    if corners == config:
        print("the default config no longer has the hot-corners block this test rewrites")
        return 1
    return run_runner(HERE / "runners" / "hot_corners.py", timeout=240, config_kdl=corners + VIRTUAL_1_OFF, output_count=2, extra_kwinrc=INSTANT_EDGES,
                      global_shortcuts=True)


if __name__ == "__main__":
    sys.exit(main())
