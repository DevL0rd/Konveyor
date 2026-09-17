import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid

Item {
    id: portal

    Layout.minimumWidth: Kirigami.Units.gridUnit * 26
    Layout.minimumHeight: Kirigami.Units.gridUnit * 20
    Layout.preferredWidth: Kirigami.Units.gridUnit * Plasmoid.configuration.popupWidth
    Layout.preferredHeight: Kirigami.Units.gridUnit * Plasmoid.configuration.popupHeight

    Loader {
        anchors.fill: parent
        active: root.popupAlive
        sourceComponent: LauncherView {
            compact: true
        }
    }
}
