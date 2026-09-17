import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import ".."

PopScroll {
    id: page

    readonly property var sections: [session, links]
    readonly property var confirmIds: ["logout", "reboot", "shutdown", "switch-user"]
    readonly property var symbolic: ({
        "lock-screen": "system-lock-screen-symbolic",
        "logout": "system-log-out-symbolic",
        "switch-user": "system-switch-user-symbolic",
        "suspend": "system-suspend-symbolic",
        "hibernate": "system-suspend-hibernate-symbolic",
        "reboot": "system-reboot-symbolic",
        "shutdown": "system-shutdown-symbolic"
    })
    property string armed: ""

    Timer {
        id: disarm
        interval: 3500
        onTriggered: page.armed = ""
    }

    SectionHeader {
        title: i18n("Session")
    }
    TileGrid {
        id: session
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.min(6, Math.floor(width / (Kirigami.Units.gridUnit * 7)))))
        cellHeight: Kirigami.Units.gridUnit * 7
        iconSize: Kirigami.Units.iconSizes.large
        model: launcherData.system
        delegate: Tile {
            id: sessionTile
            required property int index
            required property var model
            readonly property var grid: GridView.view
            readonly property string actionId: model.favoriteId || ""
            readonly property bool isArmed: page.armed === actionId
            width: grid.cellWidth
            height: grid.cellHeight
            iconSize: grid.iconSize
            monochrome: true
            iconSource: isArmed ? "dialog-warning-symbolic" : (page.symbolic[actionId] || model.decoration)
            label: isArmed ? i18n("Press again to %1", String(model.display).toLowerCase()) : model.display
            selected: GridView.isCurrentItem && grid.sectionActive
            function activate() {
                if (page.confirmIds.indexOf(actionId) >= 0 && !isArmed) {
                    page.armed = actionId
                    disarm.restart()
                    return
                }
                page.armed = ""
                launcher.trigger(grid.model, index)
            }
            function openMenu() {
                activate()
            }
            onHovered: launcher.select(grid, index)
            onClicked: activate()
        }
    }

    SectionHeader {
        title: i18n("Settings")
    }
    TileGrid {
        id: links
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / (Kirigami.Units.gridUnit * 16))))
        cellHeight: Kirigami.Units.gridUnit * 3
        model: [
            { name: i18n("System Settings"), description: i18n("Configure the whole desktop"), icon: "configure-symbolic", command: "systemsettings" },
            { name: i18n("Konveyor"), description: i18n("Tiling layout, rules and shortcuts"), icon: "view-split-left-right-symbolic", command: "kcmshell6 kcm_konveyor" },
            { name: i18n("Launcher settings"), description: i18n("Icon, size, pages and search"), icon: "start-here-kde-plasma-symbolic", command: "" },
            { name: i18n("Displays"), description: i18n("Resolution, scale and arrangement"), icon: "video-display-symbolic", command: "kcmshell6 kcm_kscreen" },
            { name: i18n("Audio"), description: i18n("Devices and volume"), icon: "audio-volume-high-symbolic", command: "kcmshell6 kcm_pulseaudio" },
            { name: i18n("Power"), description: i18n("Energy saving and sleep"), icon: "battery-symbolic", command: "kcmshell6 kcm_powerdevilprofilesconfig" }
        ]
        delegate: RowTile {
            id: linkTile
            required property int index
            required property var modelData
            readonly property var grid: GridView.view
            width: grid.cellWidth
            height: grid.cellHeight
            iconSource: modelData.icon
            monochrome: true
            label: modelData.name
            subtitle: modelData.description
            selected: GridView.isCurrentItem && grid.sectionActive
            function activate() {
                root.hide()
                if (modelData.command === "")
                    Plasmoid.internalAction("configure").trigger()
                else
                    launcherData.run(modelData.command)
            }
            function openMenu() {
                activate()
            }
            onHovered: launcher.select(grid, index)
            onClicked: activate()
        }
    }

    Item {
        Layout.fillHeight: true
    }
}
