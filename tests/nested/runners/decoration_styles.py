#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

from borderprobe import CYAN, MAGENTA, YELLOW, left_pixel, painted, painted_titles, probe_config, screenshot, settle_to
from checks import Checks, load_config
from kwinsession import for_window, frame, run_script, wait_for
from screenshot import close

RED = (255, 0, 0)
EMPTY = (0, 0, 0)
RING = 'width 8\n        active-style "Ring"'
URGENT_RING = 'width 8\n        active-style "Ring"\n        urgent-style "Alert"'
ALERT = 'border-style "Alert" {\n    layer "gradient" { colors "#ffff00"; speed 0; width 8; }\n}\n'


def ring_style(layer, name="Ring"):
    return f'border-style "{name}" {{\n    {layer}\n}}\n'


def gradient(color, extra=""):
    return f'layer "gradient" {{ colors "{color}"; speed 0; width 8; {extra} }}'


def load(styles, **options):
    return load_config(probe_config("", styles, **options))


def expect_edge(checks, title, distance, color, message):
    seen = []

    def matches():
        seen[:] = [left_pixel(screenshot(), title, distance)]
        return close(seen[0], color, 48)

    checks.expect(wait_for(matches, 20, 0.3), f"{message} ({seen})")


def blend_modes_composite_over_the_desktop(checks):
    for blend, expected, what in (("normal", MAGENTA, "paints the colour"), ("screen", MAGENTA, "adds the colour to the empty desktop"),
                                  ("multiply", EMPTY, "multiplies the empty desktop with the colour and leaves it empty")):
        checks.expect(load(ring_style(gradient("#ff00ff", f'blend "{blend}";')), ring=RING), f"a {blend} ring loads")
        settle_to("A")
        settle_to("B")
        expect_edge(checks, "B", 4, expected, f"a {blend} ring {what}")


def a_layer_without_colours_follows_the_accent(checks):
    checks.expect(load(ring_style('layer "gradient" { speed 0; width 8; }'), ring=RING), "a layer without colours loads")
    settle_to("A")
    settle_to("B")
    expect_edge(checks, "B", 4, RED, "it is painted in the KDE accent colour")


def reloading_a_style_restyles_the_ring(checks):
    checks.expect(load(ring_style(gradient("#ff00ff")), ring=RING), "a magenta ring loads")
    settle_to("B")
    expect_edge(checks, "B", 4, MAGENTA, "the focused window has the magenta ring")
    checks.expect(load(ring_style(gradient("#00ffff")), ring=RING), "the same style in cyan loads")
    expect_edge(checks, "B", 4, CYAN, "the ring takes the new colours")
    checks.expect(load(ring_style(gradient("#00ffff"), "Other"), ring='width 8\n        active-style "Ring"\n        active-color "#ffff00"'),
                  "a config that no longer defines the style loads")
    expect_edge(checks, "B", 4, YELLOW, "the ring falls back to its plain colour")


def ring_and_border_styles_are_independent(checks):
    styles = ring_style(gradient("#ff00ff")) + ring_style(gradient("#00ffff"), "Border")
    border = 'width 6\n        active-style "Border"\n        inactive-style "Border"'
    checks.expect(load(styles, ring=RING, border=border), "a styled ring and a styled border load")
    settle_to("B")
    expect_edge(checks, "B", 3, CYAN, "the border sits against the window")
    expect_edge(checks, "B", 10, MAGENTA, "and the ring outside it")


def urgent_windows_use_the_urgent_style(checks):
    checks.expect(load(ring_style(gradient("#ff00ff")) + ALERT, ring=URGENT_RING), "a ring with an urgent style loads")
    settle_to("A")
    run_script(for_window("C", "w.demandsAttention = true;"))
    expect_edge(checks, "C", 4, YELLOW, "a window asking for attention wears the urgent style")
    run_script(for_window("C", "w.demandsAttention = false;"))
    settle_to("B")


def square_windows_still_get_a_ring(checks):
    checks.expect(load(ring_style(gradient("#ff00ff")), ring=RING, radius=0), "a config with square windows loads")
    settle_to("A")
    settle_to("B")
    expect_edge(checks, "B", 4, MAGENTA, "the ring follows the square corners")


def closing_a_styled_window_hands_the_ring_on(checks):
    checks.expect(load(ring_style(gradient("#ff00ff")), ring=RING), "the magenta ring loads again")
    settle_to("C")
    expect_edge(checks, "C", 4, MAGENTA, "C wears the ring")
    run_script(for_window("C", "w.closeWindow();"))
    checks.expect(wait_for(lambda: frame("C") is None, 20), "C closed")
    checks.expect(wait_for(lambda: painted(screenshot(), "B", MAGENTA), 20, 0.3), "the ring moves to the window that took focus")
    checks.expect(not painted_titles(screenshot(), ("A",), MAGENTA), "and the other window has none")


def main():
    Checks().run(blend_modes_composite_over_the_desktop, a_layer_without_colours_follows_the_accent, reloading_a_style_restyles_the_ring,
                 ring_and_border_styles_are_independent, urgent_windows_use_the_urgent_style, square_windows_still_get_a_ring,
                 closing_a_styled_window_hands_the_ring_on)


if __name__ == "__main__":
    main()
