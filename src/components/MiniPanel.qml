import QtQuick
import org.kde.kirigami as Kirigami

Rectangle {
    id: panel

    property real shown: 0

    visible: shown > 0.01
    width: parent.width * 0.62
    height: parent.height * 0.72
    radius: Kirigami.Units.cornerRadius
    color: Kirigami.Theme.backgroundColor
    border.color: Kirigami.Theme.highlightColor
    border.width: 2
    opacity: shown
    scale: 0.9 + shown * 0.1

    Grid {
        anchors.centerIn: parent
        columns: 4
        spacing: panel.width * 0.05

        Repeater {
            model: 8

            Rectangle {
                width: panel.width * 0.14
                height: width
                radius: 3
                color: Qt.alpha(Kirigami.Theme.highlightColor, 0.35)
            }
        }
    }
}
