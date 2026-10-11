#!/usr/bin/env python3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "harness"))


def main():
    from nested import run_runner

    results = [run_runner(HERE / "runners" / "fullscreen_guard_protocol.py", timeout=150,
                          clients=("A",), extra_config="animations { off; }", output_count=outputs)
               for outputs in (1, 2)]
    return max(results)


if __name__ == "__main__":
    sys.exit(main())
