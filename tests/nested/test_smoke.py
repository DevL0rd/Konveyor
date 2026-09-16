#!/usr/bin/env python3
import sys
import textwrap
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "harness"))
from nested import NestedSession

CONFIG = textwrap.dedent("""\
    layout {
        gaps 10
        default-column-width { proportion 0.5; }
        focus-ring {
            width 4
            active-color "#1b91d5"
        }
    }

    binds {
        Mod+Right { focus-column-right; }
        Mod+Left  { focus-column-left; }
        Mod+R     { switch-preset-column-width; }
    }
""")

SESSION = textwrap.dedent("""\
    export QT_QPA_PLATFORM=wayland
    qml6 {client} -- alpha 600 400 &
    sleep 3
    qml6 {client} -- beta 600 400 &
    sleep 4
    python3 {report} > "$KONVEYOR_REPORT"
""")


def main():
    session = NestedSession(config_kdl=CONFIG)
    report = session.root / "report.json"
    client = Path(__file__).resolve().parent / "clients" / "client.qml"
    reporter = Path(__file__).resolve().parent / "harness" / "report.py"
    script = SESSION.format(client=client, report=reporter)
    session.start(f'export KONVEYOR_REPORT="{report}"\n' + script)
    try:
        session.wait(timeout=120)
    finally:
        log = session.log()

    if not report.exists():
        print(log)
        raise SystemExit("the nested session produced no report")

    import json

    data = json.loads(report.read_text())
    session.cleanup()

    assert data["effect_loaded"], f"the Konveyor effect did not load:\n{log}"
    windows = data["windows"]
    assert len(windows) == 2, f"expected two managed windows, got {windows}"

    columns = sorted(w["layout"]["pos_in_scrolling_layout"][0] for w in windows)
    assert columns == [1, 2], f"windows should sit in two columns, got {columns}"

    widths = {round(w["layout"]["tile_size"][0]) for w in windows}
    assert len(widths) == 1, f"both windows should share the default width, got {widths}"

    lefts = sorted(w["layout"]["tile_pos_in_workspace_view"][0] for w in windows)
    assert lefts[1] > lefts[0], "the second column should sit to the right of the first"

    print(f"smoke test passed: {len(windows)} windows in columns {columns}, width {widths.pop()}")


if __name__ == "__main__":
    main()
