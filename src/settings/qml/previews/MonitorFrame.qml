import QtQuick
import org.kde.kirigami as Kirigami

Rectangle {
    id: frame

    property real aspect: 16 / 9
    default property alias screenContent: screen.data
    readonly property alias screen: screen

    width: height * aspect
    radius: Kirigami.Units.cornerRadius * 2
    color: Qt.darker(Kirigami.Theme.backgroundColor, 1.6)
    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.35)
    border.width: 2

    Item {
        id: screen
        anchors.fill: parent
        anchors.margins: 5
        clip: true
    }
}
