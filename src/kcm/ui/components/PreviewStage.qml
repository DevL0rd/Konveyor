import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Rectangle {
    id: root

    property alias sourceComponent: loader.sourceComponent

    implicitHeight: Kirigami.Units.gridUnit * 13
    color: Qt.alpha(Kirigami.Theme.textColor, 0.05)

    Kirigami.Separator {
        anchors.bottom: parent.bottom
        width: parent.width
    }

    Loader {
        id: loader
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing * 2
    }
}
