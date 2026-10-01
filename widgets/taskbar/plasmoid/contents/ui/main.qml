import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore

PlasmoidItem {
    id: root
    Plasmoid.icon: "preferences-system-windows"
    Plasmoid.title: i18n("Konveyor Taskbar")
    preferredRepresentation: fullRepresentation
    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    Layout.minimumWidth: vertical ? 16 : 48
    Layout.minimumHeight: vertical ? 48 : 16
    Layout.preferredWidth: vertical ? 48 : Math.max(144, fullRepresentationItem ? fullRepresentationItem.implicitWidth : 240)
    Layout.preferredHeight: vertical ? Math.max(48, fullRepresentationItem ? fullRepresentationItem.implicitHeight : 48) : 48

    fullRepresentation: Taskbar {
        pinnedLaunchers: Plasmoid.configuration.launchers
        groupingMode: Plasmoid.configuration.groupingMode
        vertical: root.vertical
        iconSize: Math.max(16, Math.min(48, vertical ? width : height, Math.floor((vertical ? height : width) / Math.max(1, entries.length))))
        onPinsChanged: launchers => {
            Plasmoid.configuration.launchers = launchers
            Plasmoid.configuration.writeConfig()
        }
        onGroupingChanged: mode => {
            Plasmoid.configuration.groupingMode = mode
            Plasmoid.configuration.writeConfig()
        }
    }
}
