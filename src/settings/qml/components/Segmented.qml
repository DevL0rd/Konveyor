import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

RowLayout {
    id: root

    property var options: []
    property var currentValue
    property bool compact: false
    signal chosen(var value)

    spacing: 0

    QQC2.ButtonGroup {
        id: group
    }

    Repeater {
        model: root.options

        QQC2.Button {
            id: segment
            required property var modelData
            required property int index
            readonly property bool selected: root.currentValue === modelData.value

            QQC2.ButtonGroup.group: group
            text: modelData.label
            icon.name: modelData.icon || ""
            checkable: selected
            checked: selected
            Accessible.checkable: true
            Accessible.checked: selected
            Layout.fillHeight: true
            Layout.preferredWidth: root.compact ? metrics.advanceWidth + Kirigami.Units.gridUnit * 1.5 : -1
            flat: !selected
            onClicked: root.chosen(modelData.value)

            TextMetrics {
                id: metrics
                font: segment.font
                text: segment.text
            }

            QQC2.ToolTip.text: modelData.tooltip || ""
            QQC2.ToolTip.visible: hovered && (modelData.tooltip || "").length > 0
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
    }
}
