import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: preview

    property bool hideWidgets
    property bool fillPanels
    property bool noMinimize

    MonitorFrame {
        id: screen
        anchors.centerIn: parent
        height: parent.height
        aspect: 16 / 9

        Rectangle {
            id: panel
            anchors.top: parent.top
            anchors.topMargin: preview.fillPanels ? 0 : 4
            anchors.horizontalCenter: parent.horizontalCenter
            width: preview.fillPanels ? parent.width : parent.width * 0.45
            height: parent.height * 0.07
            radius: preview.fillPanels ? 0 : height / 2
            color: Qt.alpha(Kirigami.Theme.textColor, 0.35)

            Behavior on width {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutCubic
                }
            }
        }

        Repeater {
            model: 3

            Rectangle {
                required property int index
                x: parent.width * (0.08 + index * 0.1)
                y: parent.height * 0.62
                width: parent.width * 0.07
                height: width
                radius: 4
                color: Kirigami.Theme.positiveTextColor
                opacity: preview.hideWidgets ? 0 : 0.8

                Behavior on opacity {
                    NumberAnimation {
                        duration: Kirigami.Units.longDuration
                    }
                }
            }
        }

        Rectangle {
            x: parent.width * 0.4
            y: panel.height + 8
            width: parent.width * 0.56
            height: parent.height - y - 8
            radius: 4
            color: Kirigami.Theme.backgroundColor
            border.color: Kirigami.Theme.highlightColor
            border.width: 2

            Row {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 4
                spacing: 3

                Repeater {
                    model: preview.noMinimize ? ["□", "×"] : ["–", "□", "×"]

                    Text {
                        required property string modelData
                        text: modelData
                        color: Kirigami.Theme.textColor
                        font.pixelSize: 10
                    }
                }
            }
        }
    }
}
