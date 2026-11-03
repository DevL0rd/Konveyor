import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../previews"

Rectangle {
    id: root

    property alias sourceComponent: loader.sourceComponent
    property bool backdrop: false

    implicitHeight: Kirigami.Units.gridUnit * 13
    color: Qt.alpha(Kirigami.Theme.textColor, 0.05)
    clip: backdrop

    StyleBackdrop {
        anchors.fill: parent
        visible: root.backdrop
    }

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
