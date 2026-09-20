.pragma library

const pages = [
    { id: "layout", title: "Layout", icon: "view-split-left-right", file: "pages/LayoutPage.qml",
      description: "Columns, gaps, widths and where new windows land" },
    { id: "look", title: "Look", icon: "preferences-desktop-color", file: "pages/LookPage.qml",
      description: "Focus ring, borders, tabs, drop hint and corners" },
    { id: "motion", title: "Motion", icon: "preferences-desktop-effects", file: "pages/MotionPage.qml",
      description: "Animation springs, curves and speed" },
    { id: "mouse", title: "Mouse", icon: "input-mouse", file: "pages/MousePage.qml",
      description: "Focus follows mouse, hot corners and edge scrolling" },
    { id: "touch", title: "Touch & Gestures", icon: "input-touchpad", file: "pages/TouchPage.qml",
      description: "Touchpad and touchscreen swipes, pinches and long presses" },
    { id: "shortcuts", title: "Shortcuts", icon: "preferences-desktop-keyboard-shortcut", file: "pages/ShortcutsPage.qml",
      description: "Keyboard, mouse and touchpad bindings" },
    { id: "rules", title: "Window Rules", icon: "preferences-system-windows-actions", file: "pages/RulesPage.qml",
      description: "How specific apps open, size and look" },
    { id: "monitors", title: "Monitors", icon: "video-display", file: "pages/MonitorsPage.qml",
      description: "Profiles and per-monitor layout overrides" },
    { id: "workspaces", title: "Workspaces", icon: "virtual-desktops", file: "pages/WorkspacesPage.qml",
      description: "Named workspaces and switching behavior" },
    { id: "plasma", title: "Plasma Integration", icon: "plasma", file: "pages/PlasmaPage.qml",
      description: "Desktop widgets, panels, minimizing and notifications" },
    { id: "experiments", title: "Experiments", icon: "applications-science", file: "pages/ExperimentsPage.qml",
      description: "Optional fullscreen behavior still being tested" }
];

function byId(id) {
    return pages.find(page => page.id === id);
}
