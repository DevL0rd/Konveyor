import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: map

    property real screenWidth: 1920
    property real screenHeight: 1080
    property real viewTrigger: 30
    property real workspaceTrigger: 50
    property string emphasis: ""
    readonly property color viewColor: Kirigami.Theme.highlightColor
    readonly property color workspaceColor: Kirigami.Theme.positiveTextColor

    implicitWidth: Kirigami.Units.gridUnit * 16
    implicitHeight: Kirigami.Units.gridUnit * 9

    MonitorFrame {
        id: frame
        anchors.centerIn: parent
        height: Math.min(parent.height, parent.width * map.screenHeight / Math.max(1, map.screenWidth))
        aspect: map.screenWidth / Math.max(1, map.screenHeight)

        readonly property real scaleX: screen.width / Math.max(1, map.screenWidth)
        readonly property real scaleY: screen.height / Math.max(1, map.screenHeight)

        Repeater {
            model: [0, 1]

            Rectangle {
                id: sideZone
                required property int modelData
                width: Math.max(6, map.viewTrigger * frame.scaleX)
                height: parent.height
                x: modelData === 0 ? 0 : parent.width - width
                opacity: map.emphasis === "workspace" ? 0.3 : 1
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0; color: Qt.alpha(map.viewColor, sideZone.modelData === 0 ? 0.7 : 0.15) }
                    GradientStop { position: 1; color: Qt.alpha(map.viewColor, sideZone.modelData === 0 ? 0.15 : 0.7) }
                }

                Behavior on width {
                    NumberAnimation {
                        duration: Kirigami.Units.shortDuration
                    }
                }
            }
        }

        Repeater {
            model: [0, 1]

            Rectangle {
                id: edgeZone
                required property int modelData
                height: Math.max(6, map.workspaceTrigger * frame.scaleY)
                width: parent.width
                y: modelData === 0 ? 0 : parent.height - height
                opacity: map.emphasis === "view" ? 0.3 : 1
                gradient: Gradient {
                    GradientStop { position: 0; color: Qt.alpha(map.workspaceColor, edgeZone.modelData === 0 ? 0.7 : 0.15) }
                    GradientStop { position: 1; color: Qt.alpha(map.workspaceColor, edgeZone.modelData === 0 ? 0.15 : 0.7) }
                }

                Behavior on height {
                    NumberAnimation {
                        duration: Kirigami.Units.shortDuration
                    }
                }
            }
        }

        Rectangle {
            id: dragged
            width: parent.width * 0.16
            height: parent.height * 0.3
            radius: Kirigami.Units.cornerRadius
            y: parent.height * 0.35
            color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.9)
            border.color: Kirigami.Theme.highlightColor
            border.width: 2

            SequentialAnimation on x {
                loops: Animation.Infinite
                running: map.visible
                NumberAnimation { from: frame.screen.width * 0.42; to: frame.screen.width - dragged.width * 0.4; duration: 1600; easing.type: Easing.InOutQuad }
                PauseAnimation { duration: 500 }
                NumberAnimation { to: frame.screen.width * 0.42; duration: 900; easing.type: Easing.InOutQuad }
                PauseAnimation { duration: 400 }
            }

            Kirigami.Icon {
                anchors.centerIn: parent
                width: Kirigami.Units.iconSizes.small
                height: width
                source: "transform-move"
            }
        }

        Column {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: parent.width * 0.06
            anchors.topMargin: parent.height * 0.14
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: [
                    { text: "Sides scroll the row", key: "view" },
                    { text: "Top and bottom switch workspaces", key: "workspace" }
                ]

                Rectangle {
                    required property var modelData
                    width: legend.implicitWidth + Kirigami.Units.largeSpacing * 2
                    height: legend.implicitHeight + Kirigami.Units.smallSpacing * 2
                    radius: height / 2
                    color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.92)
                    border.color: modelData.key === "view" ? map.viewColor : map.workspaceColor
                    border.width: map.emphasis === modelData.key ? 2 : 1

                    QQC2.Label {
                        id: legend
                        anchors.centerIn: parent
                        text: parent.modelData.text
                        font: Kirigami.Theme.smallFont
                    }
                }
            }
        }
    }
}
