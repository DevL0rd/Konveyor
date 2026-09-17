// SPDX-FileCopyrightText: 2026 DevL0rd <dmhzmxn@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Hide Plasma desktop widgets whenever a normal, non-minimized window is
 * visible on the current virtual desktop and Activity.
 */

const service = "org.kde.plasmashell";
const path = "/PlasmaShell";
const iface = "org.kde.PlasmaShell";

let lastHidden = null;
let updateGeneration = 0;
const observedWindows = new Set();

function isOnCurrentDesktop(window) {
    return window.onAllDesktops || window.desktops.includes(workspace.currentDesktop);
}

function isOnCurrentActivity(window) {
    return window.activities.length === 0
        || window.activities.includes(workspace.currentActivity);
}

function isVisibleNormalWindow(window) {
    return window.normalWindow
        && !window.minimized
        && isOnCurrentDesktop(window)
        && isOnCurrentActivity(window);
}

function setDesktopWidgetsHidden(hidden) {
    if (hidden === lastHidden) {
        return;
    }

    lastHidden = hidden;
    const plasmaScript = `
        for (const desktop of desktops()) {
            desktop.currentConfigGroup = ["General"];
            desktop.writeConfig("hideDesktopWidgets", ${hidden});
        }
    `;

    callDBus(service, path, iface, "evaluateScript", plasmaScript);
}

function applyWindowVisibility(showingDesktop, generation) {
    if (generation !== updateGeneration) {
        return;
    }

    // Show Desktop leaves windows mapped, so it must override the window list.
    if (showingDesktop === true) {
        setDesktopWidgetsHidden(false);
        return;
    }

    setDesktopWidgetsHidden(workspace.windowList().some(isVisibleNormalWindow));
}

function updateVisibility() {
    const generation = ++updateGeneration;
    callDBus(
        "org.kde.KWin",
        "/KWin",
        "org.freedesktop.DBus.Properties",
        "Get",
        "org.kde.KWin",
        "showingDesktop",
        showingDesktop => applyWindowVisibility(showingDesktop, generation)
    );
}

function observeWindow(window) {
    if (observedWindows.has(window)) {
        return;
    }

    observedWindows.add(window);
    window.minimizedChanged.connect(updateVisibility);
    window.desktopsChanged.connect(updateVisibility);
    window.activitiesChanged.connect(updateVisibility);
    window.closed.connect(() => {
        observedWindows.delete(window);
        updateVisibility();
    });
}

workspace.windowList().forEach(observeWindow);
workspace.windowAdded.connect(window => {
    observeWindow(window);
    updateVisibility();
});
workspace.windowRemoved.connect(updateVisibility);
workspace.windowActivated.connect(updateVisibility);
workspace.currentDesktopChanged.connect(updateVisibility);
workspace.currentActivityChanged.connect(updateVisibility);

updateVisibility();
