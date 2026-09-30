#!/usr/bin/env python3
"""Resize tiled windows by dragging each of their edges with Meta and the right mouse button."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import REPO, run_runner

    config = (REPO / "data" / "default-config.kdl").read_text()
    resizable = config.replace("// resize-tiled-windows", "resize-tiled-windows").replace(
        "    default-column-width { proportion 0.5; }", "    default-column-width { proportion 0.3; }", 1)
    if "proportion 0.3;" not in resizable or "// resize-tiled-windows" in resizable:
        print("the default config no longer has the lines this test rewrites")
        return 1
    return run_runner(HERE / "runners" / "resize_edges.py", timeout=240, config_kdl=resizable)


if __name__ == "__main__":
    sys.exit(main())
