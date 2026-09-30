pragma Singleton
import QtQuick

QtObject {
    property var triggered: []
    property var favoriteClients: []
    property var categories: [
        { display: "All Applications", apps: ["org.kde.konsole.desktop", "org.kde.dolphin.desktop", "firefox.desktop", "steam_app_620.desktop"] },
        { display: "Utilities", apps: ["org.kde.konsole.desktop"] }
    ]
    property var apps: ({
        "org.kde.konsole.desktop": { display: "Konsole", decoration: "utilities-terminal", description: "Terminal", url: "file:///usr/share/applications/org.kde.konsole.desktop" },
        "org.kde.dolphin.desktop": { display: "Dolphin", decoration: "system-file-manager", description: "File Manager", url: "file:///usr/share/applications/org.kde.dolphin.desktop" },
        "firefox.desktop": { display: "Firefox", decoration: "firefox", description: "Web Browser", url: "file:///usr/share/applications/firefox.desktop", isNewlyInstalled: true },
        "steam_app_620.desktop": { display: "Portal 2", decoration: "steam_icon_620", description: "Game", url: "file:///home/user/.local/share/applications/steam_app_620.desktop" }
    })
    property var favorites: ["org.kde.konsole.desktop", "org.kde.dolphin.desktop"]
    property var system: [
        { display: "Lock", decoration: "system-lock-screen", favoriteId: "lock-screen" },
        { display: "Log Out", decoration: "system-log-out", favoriteId: "logout" },
        { display: "Sleep", decoration: "system-suspend", favoriteId: "suspend" },
        { display: "Restart", decoration: "system-reboot", favoriteId: "reboot" },
        { display: "Shut Down", decoration: "system-shutdown", favoriteId: "shutdown" }
    ]
    property var places: [
        { display: "Home", decoration: "user-home", url: "file:///home/user" },
        { display: "Network", decoration: "network-workgroup", url: "" },
        { display: "Trash", decoration: "user-trash", url: "trash:/" }
    ]
    property var recentApps: ["firefox.desktop"]
    property var recentDocs: [
        { display: "notes.txt", decoration: "text-plain", url: "file:///home/user/notes.txt", favoriteId: "file:///home/user/notes.txt" }
    ]
    property var recentFolders: [
        { display: "Projects", decoration: "folder", url: "file:///home/user/Projects", favoriteId: "file:///home/user/Projects" }
    ]
    property var runnerGroups: [
        { name: "Applications", runner: "krunner_services", apps: true },
        { name: "Calculator", runner: "calculator", rows: [{ display: "4", decoration: "accessories-calculator" }] },
        { name: "Command Line", runner: "krunner_shell", rows: [{ display: "Run ls", decoration: "utilities-terminal" }] }
    ]

    function app(id) {
        return Object.assign({ favoriteId: id, display: id }, apps[id] || {})
    }
    function appRows(ids) {
        return ids.map(id => app(id))
    }
}
