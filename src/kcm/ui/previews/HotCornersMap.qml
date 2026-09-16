import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: map

    property var corners: ({})
    property bool active: true
    property real aspect: 16 / 9
    signal toggled(string corner)

    readonly property var cornerList: [
        { id: "top-left", label: "Top left", h: 0, v: 0 },
        { id: "top-right", label: "Top right", h: 1, v: 0 },
        { id: "bottom-left", label: "Bottom left", h: 0, v: 1 },
        { id: "bottom-right", label: "Bottom right", h: 1, v: 1 }
    ]

    implicitWidth: Kirigami.Units.gridUnit * 16
    implicitHeight: Kirigami.Units.gridUnit * 9

    MonitorFrame {
        id: frame
        anchors.centerIn: parent
        height: Math.min(parent.height, parent.width / map.aspect)
        aspect: map.aspect
        opacity: map.active ? 1 : 0.45

        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.5
            height: parent.height * 0.55
            radius: Kirigami.Units.cornerRadius
            color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.5)
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.15)

            QQC2.Label {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.largeSpacing
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: map.active ? "Push the pointer into a lit corner to open the overview" : "Hot corners are off"
                font: Kirigami.Theme.smallFont
                opacity: 0.8
            }
        }

        Repeater {
            model: map.cornerList

            QQC2.AbstractButton {
                id: corner

                required property var modelData
                readonly property bool on: map.active && map.corners[modelData.id] === true

                width: Kirigami.Units.gridUnit * 3.2
                height: width
                x: modelData.h === 0 ? -width / 2 : parent.width - width / 2
                y: modelData.v === 0 ? -height / 2 : parent.height - height / 2
                enabled: map.active
                hoverEnabled: true
                onClicked: map.toggled(modelData.id)

                Accessible.role: Accessible.CheckBox
                Accessible.name: modelData.label + " corner"
                Accessible.checked: on

                QQC2.ToolTip.text: modelData.label + (on ? ": on" : ": off")
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

                background: Rectangle {
                    radius: width / 2
                    color: corner.on ? Qt.alpha(Kirigami.Theme.highlightColor, corner.hovered ? 0.75 : 0.55)
                                     : Qt.alpha(Kirigami.Theme.textColor, corner.hovered ? 0.18 : 0.06)
                    border.color: corner.on ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.3)
                    border.width: corner.hovered ? 2 : 1

                    SequentialAnimation on scale {
                        running: corner.on
                        loops: Animation.Infinite
                        alwaysRunToEnd: true
                        NumberAnimation { from: 1; to: 1.12; duration: 900; easing.type: Easing.InOutSine }
                        NumberAnimation { from: 1.12; to: 1; duration: 900; easing.type: Easing.InOutSine }
                    }

                    Behavior on color {
                        ColorAnimation {
                            duration: Kirigami.Units.shortDuration
                        }
                    }
                }
            }
        }
    }
}
