#!/usr/bin/env python3
"""Rest the pointer at screen edges on two outputs and check edge-switch-columns steps one column per visit, at outer and shared edges."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import REPO, run_script

    config = (REPO / "data" / "default-config.kdl").read_text()
    enabled = config.replace("edge-switch-columns { off;", "edge-switch-columns { on;")
    if enabled == config:
        print("the default config no longer has the edge-switch-columns line this test rewrites")
        return 1
    script = f'export QT_QPA_PLATFORM=wayland\npython3 {HERE / "runners" / "edge_switch_columns.py"} > "$KONVEYOR_REPORT" 2>&1\n'
    return run_script(script, timeout=300, config_kdl=enabled, output_count=2)


if __name__ == "__main__":
    sys.exit(main())
