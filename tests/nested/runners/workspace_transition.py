#!/usr/bin/env python3
"""Check that a workspace switch, by gesture or keyboard, moves once and that the
focus ring follows the live workspace position according to focus-ring-transition."""
import json
import math
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "harness"))

import virtualtouchpad as touchpad
from borderprobe import probe_config, screenshot
from checks import Checks, load_config
from fakepointer import move
from kwinsession import konveyor, konveyor_action, open_client, qdbus, run_script, wait_for

RING_STYLE = 'border-style "Ring" {\n    layer "gradient" { colors "#ff00ff"; speed 0; width 8; }\n}\n'
RING = 'width 8\n        active-style "Ring"'
SWIPE_STEPS = 4
SWIPE_DELTA = 45
SETTLE_SECONDS = 1.9
WINDOW_COLORS = {"A": (200, 0, 0), "B": (0, 200, 0)}


def ring_config(mode, duration_ms=1200):
    animations = f'workspace-switch {{ duration-ms {duration_ms}; curve "linear"; }}'
    config = probe_config("", RING_STYLE, ring=RING, animations=animations)
    return config.replace('focus-ring-transition "ride"', f'focus-ring-transition "{mode}"')


def all_frames():
    printed = run_script('for (const w of workspace.windowList()) { if (!w.deleted && w.normalWindow) { const g = w.frameGeometry; '
                         'print("MARK|" + w.caption + "|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height); } }')
    result = {}
    for line in printed:
        title, *values = line.split("|")
        result[title] = tuple(float(value) for value in values)
    return result


def active_workspace():
    return next(workspace["idx"] for workspace in json.loads(konveyor("Workspaces")) if workspace["is_active"])


def kde_active_effects():
    return qdbus("org.kde.KWin", "/Effects", "org.freedesktop.DBus.Properties.Get", "org.kde.kwin.Effects", "activeEffects").split()


def span(start, end, count=7, limit=None):
    low = max(0, math.ceil(start))
    high = math.floor(end)
    if limit is not None:
        high = min(high, limit - 1)
    if high < low:
        return []
    if high == low:
        return [low]
    return sorted({low + round((high - low) * step / (count - 1)) for step in range(count)})


def interior_owner(image, geometry):
    """Which colored window shows through the middle of this frame, if any."""
    x, y, width, height = geometry
    left, right = max(0, int(x) + 40), min(image.width - 1, int(x + width) - 40)
    top, bottom = max(0, int(y) + 40), min(image.height - 1, int(y + height) - 40)
    if right < left or bottom < top:
        return None
    votes = {"A": 0, "B": 0}
    for column in (left, (left + right) // 2, right):
        for row in (top, (top + bottom) // 2, bottom):
            red, green, blue = image.getpixel((column, row))[:3]
            if red > 80 and green < 80 and blue < 80:
                votes["A"] += 1
            elif green > 80 and red < 80 and blue < 80:
                votes["B"] += 1
    owner = max(votes, key=votes.get)
    return owner if votes[owner] else None


def magenta(image, column, row):
    column, row = int(column), int(row)
    if not (0 <= column < image.width and 0 <= row < image.height):
        return 0.0
    red, green, blue = image.getpixel((column, row))[:3]
    strength = min(red, blue)
    return float(strength) if green < strength * 0.7 else 0.0


def ring_score(image, geometry, title):
    """Brightest magenta-ish pixel on this window's visible edge, attributed by content color."""
    if geometry is None or interior_owner(image, geometry) != title:
        return 0.0
    x, y, width, height = geometry
    columns = span(x + 6, x + width - 7, limit=image.width)
    rows = span(y + 6, y + height - 7, limit=image.height)
    best = 0.0
    for distance in range(2, 12):
        for row in rows:
            best = max(best, magenta(image, x - distance, row), magenta(image, x + width + distance - 1, row))
        for column in columns:
            best = max(best, magenta(image, column, y - distance), magenta(image, column, y + height + distance - 1))
    return best


def measure(image, frames):
    return {title: ring_score(image, frames.get(title), title) for title in ("A", "B")}


def sample_once():
    frames = all_frames()
    return frames, measure(screenshot(), frames)


def focus_workspace(index):
    return konveyor_action("focus-workspace", str(index))


def on_workspace(index):
    return wait_for(lambda: active_workspace() == index, 20)


def no_slide():
    return wait_for(lambda: "slide" not in kde_active_effects(), 10)


def watch_effects(stop, seen):
    while not stop.is_set():
        try:
            effects = tuple(kde_active_effects())
        except Exception:
            continue
        if not seen or seen[-1] != effects:
            seen.append(effects)
        time.sleep(0.02)


def slide_seen(seen):
    return any("slide" in effects for effects in seen)


def partial_swipe():
    """Move part of a workspace, then release so the settle animation stays visible."""
    touchpad.swipe_begin(3)
    for _ in range(SWIPE_STEPS):
        touchpad.swipe_update(0, -SWIPE_DELTA)
        time.sleep(0.05)
    touchpad.swipe_end()


def gesture_records():
    stop = threading.Event()
    seen = []
    watcher = threading.Thread(target=watch_effects, args=(stop, seen), daemon=True)
    watcher.start()
    partial_swipe()
    started = time.monotonic()
    records = []
    while time.monotonic() - started < SETTLE_SECONDS:
        frames = all_frames()
        image = screenshot()
        records.append((time.monotonic() - started, frames, measure(image, frames)))
    stop.set()
    watcher.join(5)
    return records, seen


def keyboard_switch_records():
    """Call the keyboard switch while sampling the screen and the active effects."""
    stop = threading.Event()
    seen = []
    watcher = threading.Thread(target=watch_effects, args=(stop, seen), daemon=True)
    watcher.start()
    with ThreadPoolExecutor(max_workers=1) as pool:
        pending = pool.submit(focus_workspace, 2)
        started = time.monotonic()
        records = []
        while time.monotonic() - started < SETTLE_SECONDS:
            frames = all_frames()
            image = screenshot()
            records.append((time.monotonic() - started, frames, measure(image, frames)))
        pending.result()
    stop.set()
    watcher.join(5)
    return records, seen


def reversing_swipe():
    """Swipe toward the next workspace, pull back to the start, then release."""
    touchpad.swipe_begin(3)
    for _ in range(SWIPE_STEPS):
        touchpad.swipe_update(0, -SWIPE_DELTA)
        time.sleep(0.05)
    for _ in range(SWIPE_STEPS):
        touchpad.swipe_update(0, SWIPE_DELTA)
        time.sleep(0.05)
    time.sleep(0.25)
    touchpad.swipe_end()


def closest(records, target):
    return min(records, key=lambda record: abs(record[0] - target))


def prepare(checks):
    checks.expect(load_config(ring_config("ride")), "a config with a magenta focus ring loads")
    checks.equal(focus_workspace(1), "", "start on the first workspace")
    open_client("A", client="colored-client.qml")
    checks.expect(wait_for(lambda: "A" in all_frames(), 60), "the red window is open")
    time.sleep(0.5)
    checks.equal(focus_workspace(2), "", "switch to the second workspace")
    checks.expect(on_workspace(2), "the second workspace is active")
    open_client("B", client="colored-client.qml")
    checks.expect(wait_for(lambda: "B" in all_frames(), 60), "the green window is open")
    time.sleep(0.5)
    checks.equal(focus_workspace(1), "", "return to the first workspace")
    checks.expect(on_workspace(1), "the first workspace is active")
    time.sleep(0.5)
    move(960, 540)
    touchpad.add()
    _, scores = sample_once()
    checks.expect(scores["A"] > 0, f"the active window shows its focus ring ({scores})")
    return scores["A"]


def settles_moving_up_once(checks, records, gesture):
    checks.expect(on_workspace(2), f"the {gesture} settles on the second workspace")
    y = [sample[1]["A"][1] for sample in records if "A" in sample[1] and -1200 < sample[1]["A"][1] < -50]
    drops = [later - earlier for earlier, later in zip(y, y[1:])]
    checks.expect(len(y) > 3, f"the outgoing window was sampled mid-transition ({y})")
    checks.expect(all(delta <= 2 for delta in drops), f"the outgoing window moves up once without bouncing back ({drops})")
    checks.expect(y and y[0] - y[-1] >= 250, f"the outgoing window travels up the workspace ({y[0]} -> {y[-1]})")


def gesture_moves_once(checks):
    """A gesture switch must not also hand the desktop to KWin Slide."""
    checks.expect(load_config(ring_config("ride")), "the ride config loads again")
    checks.expect(no_slide(), "KWin Slide is idle before the swipe")
    records, seen = gesture_records()
    checks.expect(not slide_seen(seen), f"a gesture switch does not start KWin Slide ({seen})")
    settles_moving_up_once(checks, records, "swipe")
    wait_for(lambda: sample_once()[1]["B"] > 0, 20, 0.2)
    checks.expect(sample_once()[1]["B"] > 0, "the destination window keeps its focus ring after the swipe")


def ring_handoff(checks, mode, full):
    checks.expect(load_config(ring_config(mode)), f"the {mode} ring config loads")
    checks.equal(focus_workspace(1), "", f"return to the first workspace for the {mode} swipe")
    checks.expect(on_workspace(1), "the first workspace is active")
    time.sleep(1.5)
    checks.expect(no_slide(), "KWin Slide is idle before the swipe")
    records, seen = gesture_records()
    checks.expect(not slide_seen(seen), f"a {mode} gesture switch leaves KWin Slide out ({seen})")
    mid = closest(records, 0.2)
    first = closest(records, 0.05)
    checks.expect(0.05 <= mid[0] <= 0.45, f"a {mode} screenshot was taken mid-transition ({mid[0]:.2f}s)")
    outgoing, incoming = mid[2]["A"], mid[2]["B"]
    print(f"ring {mode}: first={first[2]} mid={mid[2]} at {mid[0]:.2f}s full={full:.0f}")
    if mode == "ride":
        checks.expect(outgoing >= full * 0.55 and incoming >= full * 0.55,
                      f"ride keeps the old ring while the new one is already there ({outgoing:.0f}, {incoming:.0f} of {full:.0f})")
    elif mode == "crossfade":
        checks.expect(outgoing >= full * 0.15 and outgoing <= full * 0.85 and incoming >= full * 0.15
                          and incoming <= full * 0.85,
                      f"crossfade shares the ring between both windows ({outgoing:.0f}, {incoming:.0f} of {full:.0f})")
        checks.expect(first[2]["A"] >= full * 0.15, f"crossfade keeps the outgoing ring visible as it leaves ({first[2]})")
    else:
        checks.expect(outgoing <= full * 0.12 and incoming <= full * 0.12,
                      f"delayed hides both rings while the workspace moves ({outgoing:.0f}, {incoming:.0f} of {full:.0f})")
        checks.expect(first[2]["A"] <= full * 0.12, f"delayed removes the old ring immediately ({first[2]})")
    checks.expect(on_workspace(2), f"the {mode} swipe settles on the second workspace")
    time.sleep(0.6)
    _, measured = sample_once()
    checks.expect(measured["B"] >= full * 0.55, f"the new window wears its ring once the swipe settles ({measured})")
    checks.expect(measured["A"] <= full * 0.12, f"the old window drops its ring once the swipe settles ({measured})")


def keyboard_switch_paints_once(checks, full):
    """Konveyor paints the keyboard switch itself, so Slide must stay out of it."""
    checks.expect(load_config(ring_config("delayed")), "the delayed config loads for the keyboard switch")
    checks.equal(focus_workspace(1), "", "return to the first workspace")
    checks.expect(on_workspace(1), "the first workspace is active")
    time.sleep(1.5)
    checks.expect(no_slide(), "KWin Slide is idle before the keyboard switch")
    records, seen = keyboard_switch_records()
    checks.expect(not slide_seen(seen), f"a keyboard switch does not start KWin Slide ({seen})")
    settles_moving_up_once(checks, records, "keyboard switch")
    moving = [record for record in records if record[0] > 0.3 and -1200 < record[1].get("A", (0, 0))[1] < -50]
    checks.expect(bool(moving), "the keyboard switch was watched while the workspace was moving")
    checks.expect(all(record[2]["B"] <= full * 0.12 for record in moving),
                  f"delayed keeps the incoming ring back while the switch moves ({[record[2] for record in moving]})")
    checks.expect(wait_for(lambda: sample_once()[1]["B"] >= full * 0.55, 5, 0.1),
                  "the incoming ring arrives once the keyboard switch settles")


def partial_swipe_returns_the_ring(checks, full):
    """Pulling a swipe back must put the ring back on the window it started from."""
    checks.expect(load_config(ring_config("crossfade")), "the crossfade config loads for the reversing swipe")
    checks.equal(focus_workspace(1), "", "return to the first workspace")
    checks.expect(on_workspace(1), "the first workspace is active")
    time.sleep(1.5)
    reversing_swipe()
    checks.expect(on_workspace(1), "a swipe pulled back settles on the first workspace")
    time.sleep(0.6)
    _, settled = sample_once()
    checks.expect(settled["A"] >= full * 0.55, f"the original window gets its ring back ({settled})")
    checks.expect(settled["B"] <= full * 0.12, f"the other window never keeps a focused ring ({settled})")


def main():
    state = {}
    checks = Checks()
    checks.run(
        lambda each: state.setdefault("full", prepare(each)),
        gesture_moves_once,
        lambda each: ring_handoff(each, "ride", state["full"]),
        lambda each: ring_handoff(each, "crossfade", state["full"]),
        lambda each: ring_handoff(each, "delayed", state["full"]),
        lambda each: partial_swipe_returns_the_ring(each, state["full"]),
        lambda each: keyboard_switch_paints_once(each, state["full"]),
    )
    touchpad.remove()


if __name__ == "__main__":
    main()
