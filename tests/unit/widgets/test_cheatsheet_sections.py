#!/usr/bin/env python3
import tempfile
import unittest

from cheatsheet_module import load_module


def bind(key, name, arguments=(), **extra):
    return {"key": key, "action": {"name": name, "arguments": list(arguments), "properties": extra.pop("properties", {})}, **extra}


def gesture(motion, action, fingers=3, device="touchpad", **extra):
    return {"device": device, "fingers": fingers, "motion": motion, "action": action, "natural": True, **extra}


class TestCheatsheetSections(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.module, _ = load_module(self.temporary.name)

    def tearDown(self):
        self.temporary.cleanup()

    def sections(self, binds, gestures=(), kde=""):
        self.module.konveyor_binds = lambda: binds
        self.module.konveyor_gestures = lambda: list(gestures)
        self.module.KGLOBALSHORTCUTS.write_text(kde)
        return self.module.build_sections()

    def test_describes_a_bind_by_its_title(self):
        self.assertEqual(self.module.describe_konveyor_bind(bind("Mod+T", "spawn", ["konsole"], title="Terminal")), "Terminal")

    def test_describes_a_bind_by_its_action(self):
        described = self.module.describe_konveyor_bind(bind("Mod+R", "set-column-width", ["+10%"], properties={"id": "3"}))
        self.assertEqual(described, "Set column width +10% id=3")
        self.assertEqual(self.module.describe_konveyor_bind({"key": "Mod+Q", "action": {"name": "close-window"}}), "Close window")

    def test_sorts_actions_into_categories(self):
        expected = {
            "move-column-to-workspace-down": "Workspaces",
            "focus-monitor-left": "Monitors",
            "toggle-window-floating": "Floating & Tabs",
            "toggle-column-tabbed-display": "Floating & Tabs",
            "set-column-width": "Sizing",
            "fullscreen-window": "Sizing",
            "center-column": "Sizing",
            "move-column-left": "Move Windows",
            "consume-window-into-column": "Move Windows",
            "swap-window-right": "Move Windows",
            "focus-column-left": "Focus",
            "close-window": "Konveyor",
        }
        self.assertEqual({name: self.module.konveyor_category(name) for name in expected}, expected)

    def test_groups_numbered_binds(self):
        self.assertEqual(self.module.numbered_group(bind("Mod+3", "focus-workspace", ["3"])), ("focus-workspace", "Mod+"))
        self.assertIsNone(self.module.numbered_group(bind("Mod+F3", "focus-workspace", ["4"])))
        self.assertIsNone(self.module.numbered_group(bind("Mod+3", "set-column-width", ["+3"])))
        self.assertIsNone(self.module.numbered_group(bind("Mod+3", "focus-workspace", ["3", "x"])))

    def test_konveyor_sections_merge_hide_and_number(self):
        binds = [
            bind("Mod+Left", "focus-column-left"),
            bind("Mod+H", "focus-column-left"),
            bind("Mod+X", "focus-column-right", hidden=True),
            *[bind(f"Mod+{number}", "focus-workspace", [str(number)]) for number in range(1, 10)],
            *[bind(f"Mod+Shift+{number}", "move-column-to-workspace", [str(number)]) for number in (2, 5)],
        ]
        sections = self.module.konveyor_sections(binds)
        self.assertEqual(sections["Focus"], [{"action": "Focus column left", "keys": ["Mod+Left", "Mod+H"], "id": "focus-column-left"}])
        self.assertEqual(sections["Workspaces"], [
            {"action": "Focus workspace 1–9", "keys": ["Mod+1…9"], "id": "focus-workspace"},
            {"action": "Move column to workspace 2–5", "keys": ["Mod+Shift+2…5"], "id": "move-column-to-workspace"},
        ])

    def test_describes_every_gesture(self):
        entries = [self.module.gesture_entry(item) for item in [
            gesture("swipe-horizontal", "scroll-view"),
            gesture("swipe-vertical", "switch-workspace", 4),
            gesture("pinch", "toggle-overview", 4),
            gesture("window-swipe-horizontal", "consume-or-expel", 3, "touchscreen"),
            gesture("window-swipe-vertical", "move-window", 3, "touchscreen"),
            gesture("tap", "cycle-width"),
            gesture("tap", "kontrol-panel", 4),
            gesture("tap", "toggle-overview", 5, "touchscreen"),
            gesture("long-press", "move-window", 1, "touchscreen", **{"hold-ms": 450}),
        ]]
        self.assertEqual([(entry["action"], entry["keys"]) for entry in entries], [
            ("Scroll the row on the touchpad", ["3 fingers swipe ←→"]),
            ("Switch workspace on the touchpad", ["4 fingers swipe ↑↓"]),
            ("Open or close the overview on the touchpad", ["4 fingers pinch"]),
            ("Merge the window into the next column, or pop it out on the touchscreen", ["3 fingers swipe ←→"]),
            ("Move the window up or down, then to the next workspace on the touchscreen", ["3 fingers swipe ↑↓"]),
            ("Cycle the column width on the touchpad", ["3 fingers tap"]),
            ("Open or close the Kontrol Panel on the touchpad", ["4 fingers tap"]),
            ("Open or close the overview on the touchscreen", ["5 fingers tap"]),
            ("Move a window: hold its title bar on the touchscreen", ["Hold 0.45 s+Drag"]),
        ])

    def test_shows_gestures_it_does_not_know_by_their_names(self):
        tap = self.module.gesture_entry(gesture("tap", "show-desktop", 4))
        self.assertEqual((tap["action"], tap["keys"]), ("Show desktop on the touchpad", ["4 fingers tap"]))
        edge = self.module.gesture_entry(gesture("edge-swipe", "open-launcher", 2, "touchscreen"))
        self.assertEqual((edge["action"], edge["keys"]), ("Open launcher on the touchscreen", ["2 fingers edge swipe"]))

    def test_widget_entries_need_the_portal_launcher_plasmoid(self):
        self.assertEqual(self.module.widget_entries(), [])
        (self.module.DATA_HOME / "plasma" / "plasmoids" / "org.devl0rd.portal.launcher").mkdir(parents=True)
        self.assertEqual(self.module.widget_entries(), [{"action": "Kontrol Panel: jump to a page", "keys": ["Alt+1…8"]}])

    def test_collapses_numbered_kde_entries(self):
        entries = [{"action": f"Switch to Desktop {number}", "keys": [f"Ctrl+F{number}"]} for number in (1, 2, 3)]
        entries += [{"action": "Window to Desktop 7", "keys": ["Ctrl+Shift+7"]}, {"action": "Screen 2", "keys": ["Meta+A", "Meta+B"]}]
        self.assertEqual(self.module.collapse_numbered(entries), [
            {"action": "Switch to Desktop 1–3", "keys": ["Ctrl+F1…3"]},
            {"action": "Window to Desktop 7", "keys": ["Ctrl+Shift+7"]},
            {"action": "Screen 2", "keys": ["Meta+A", "Meta+B"]},
        ])

    def test_orders_sections_and_sorts_entries(self):
        (self.module.DATA_HOME / "plasma" / "plasmoids" / "org.devl0rd.portal.launcher").mkdir(parents=True)
        sections = self.sections(
            [bind("Mod+Q", "close-window"), bind("Mod+W", "focus-monitor-left"), bind("Mod+L", "focus-column-right"),
             bind("Mod+F", "toggle-window-floating"), bind("Mod+K", "focus-column-left"), bind("Mod+Ctrl+L", "move-column-right"),
             bind("Mod+R", "set-column-width", ["+10%"]), bind("Mod+1", "focus-workspace", ["1"])],
            [gesture("pinch", "toggle-overview", 4)],
            "[org_kde_powerdevil]\nSleep=Meta+S,none,Suspend\n[kwin]\nShow Desktop=Meta+Q\\tMeta+D,none,Peek at Desktop\n"
            "[konveyor-kontrol-panel]\ntoggle=Meta,Meta,Open\n")
        self.assertEqual([section["name"] for section in sections], [
            "Focus", "Move Windows", "Sizing", "Workspaces", "Floating & Tabs", "Monitors", "Konveyor", "Gestures", "Konveyor widgets",
            "KDE Windows & Desktops", "KDE Power",
        ])
        self.assertEqual([entry["action"] for entry in sections[0]["entries"]], ["Focus column left", "Focus column right"])
        self.assertEqual(sections[8]["entries"], [{"action": "Kontrol Panel: jump to a page", "keys": ["Alt+1…8"]},
                                                  {"action": "Kontrol Panel: open", "keys": ["Meta"]}])
        self.assertEqual(sections[9]["entries"], [{"action": "Peek at Desktop", "keys": ["Meta+D"]}])

    def test_an_empty_config_gives_no_sections(self):
        self.assertEqual(self.sections([]), [])


if __name__ == "__main__":
    unittest.main()
