import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: task

    property Item strip
    property var icons: []
    property bool capsule: false
    property int count: 1
    property int focusedIcon: -1
    property int dimmedIcon: -1
    property int clickedIcon: -1
    property int clickSerial: 0
    property string badge

    readonly property real button: strip.iconSize + strip.padding * 2
    readonly property real inset: capsule ? Math.max(1, strip.padding / 2) : 0

    width: row.width + inset * 2
    height: strip.thickness

    Behavior on width {
        NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
    }

    Rectangle {
        visible: task.capsule
        anchors.centerIn: parent
        width: parent.width
        height: task.button + task.inset * 2
        radius: Kirigami.Units.cornerRadius * 1.5
        color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
        border.width: 1
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.14)
    }

    Row {
        id: row
        anchors.centerIn: parent

        Repeater {
            model: task.icons

            Item {
                id: slot
                required property string modelData
                required property int index
                readonly property bool focused: task.focusedIcon === index
                width: task.button
                height: task.strip.thickness

                Rectangle {
                    anchors.centerIn: parent
                    width: task.button - 2
                    height: task.button - 2
                    radius: Kirigami.Units.cornerRadius
                    color: task.strip.look.highlightStyle === 0 && slot.focused ? Qt.alpha(Kirigami.Theme.highlightColor, 0.28) : "transparent"
                    border.width: slot.focused && task.strip.look.highlightStyle !== 2 ? 1.5 : 0
                    border.color: Kirigami.Theme.highlightColor

                    Behavior on color {
                        ColorAnimation { duration: Kirigami.Units.shortDuration }
                    }
                }

                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: task.strip.iconSize
                    height: width
                    source: slot.modelData
                    opacity: task.dimmedIcon === slot.index ? 0.35 : 1

                    Behavior on opacity {
                        NumberAnimation { duration: Kirigami.Units.longDuration }
                    }
                }

                Rectangle {
                    id: ring
                    anchors.centerIn: parent
                    width: task.strip.iconSize * (0.4 + ringProgress * 0.9)
                    height: width
                    radius: width / 2
                    color: "transparent"
                    border.width: 2
                    border.color: Kirigami.Theme.highlightColor
                    opacity: task.clickedIcon === slot.index ? 1 - ringProgress : 0
                    property real ringProgress: 1

                    NumberAnimation on ringProgress {
                        id: ripple
                        from: 0
                        to: 1
                        duration: 600
                        running: false
                    }

                    Connections {
                        target: task
                        function onClickSerialChanged() {
                            if (task.clickedIcon === slot.index)
                                ripple.restart()
                        }
                    }
                }

                Row {
                    visible: task.strip.look.indicatorStyle !== 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: task.strip.opposite ? 1 : parent.height - height - 1
                    spacing: 2

                    Repeater {
                        model: task.strip.look.indicatorStyle === 1 ? 1 : (task.capsule ? 1 : task.count)

                        Rectangle {
                            readonly property real dot: Math.max(2, task.strip.thickness * 0.07)
                            width: task.strip.look.indicatorStyle === 1 ? task.strip.iconSize * (slot.focused ? 0.5 : 0.28) : (slot.focused && index === 0 ? dot * 3 : dot)
                            height: dot
                            radius: dot / 2
                            color: slot.focused ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
                            opacity: slot.focused ? 1 : 0.55

                            Behavior on width {
                                NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
                            }
                        }
                    }
                }
            }
        }
    }

    MiniBadge {
        label: task.badge
        size: task.strip.thickness * 0.34
        x: parent.width - width * 0.75
        y: (parent.height - task.button) / 2 - height * 0.25
    }
}
