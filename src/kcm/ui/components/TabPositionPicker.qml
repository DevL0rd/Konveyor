import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: picker

    property string position: "left"
    property bool inside: false
    signal chosen(string position)

    implicitWidth: Kirigami.Units.gridUnit * 9
    implicitHeight: Kirigami.Units.gridUnit * 6

    Rectangle {
        id: column
        anchors.centerIn: parent
        width: parent.width * 0.45
        height: parent.height * 0.62
        radius: 4
        color: Qt.alpha(Kirigami.Theme.textColor, 0.1)
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.3)

        QQC2.Label {
            anchors.centerIn: parent
            text: "Tabs"
            font: Kirigami.Theme.smallFont
            opacity: 0.6
        }
    }

    Repeater {
        model: ["left", "right", "top", "bottom"]

        QQC2.AbstractButton {
            id: side

            required property string modelData
            readonly property bool vertical: modelData === "left" || modelData === "right"
            readonly property bool selected: picker.position === modelData
            readonly property real reach: Kirigami.Units.gridUnit * 1.1

            hoverEnabled: true
            width: vertical ? reach : column.width
            height: vertical ? column.height : reach
            x: modelData === "left" ? column.x - reach : modelData === "right" ? column.x + column.width : column.x
            y: modelData === "top" ? column.y - reach : modelData === "bottom" ? column.y + column.height : column.y
            onClicked: picker.chosen(modelData)
            Accessible.name: "Tabs on the " + modelData
            Accessible.role: Accessible.RadioButton
            Accessible.checked: selected

            contentItem: Item {
                Rectangle {
                    readonly property real thickness: side.selected ? 5 : 3
                    width: side.vertical ? thickness : parent.width * 0.7
                    height: side.vertical ? parent.height * 0.7 : thickness
                    radius: 2
                    x: side.vertical ? (side.modelData === "left" ? (picker.inside && side.selected ? side.reach + 3 : side.reach - thickness - 3) : (picker.inside && side.selected ? -thickness - 3 : 3)) : (parent.width - width) / 2
                    y: side.vertical ? (parent.height - height) / 2 : (side.modelData === "top" ? (picker.inside && side.selected ? side.reach + 3 : side.reach - thickness - 3) : (picker.inside && side.selected ? -thickness - 3 : 3))
                    color: side.selected ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, side.hovered ? 0.5 : 0.2)

                    Behavior on x {
                        NumberAnimation {
                            duration: Kirigami.Units.shortDuration
                        }
                    }
                }
            }
        }
    }
}
