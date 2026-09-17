import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

RowLayout {
    id: root

    property var options: []
    property var currentValue
    signal chosen(var value)

    spacing: 0

    Repeater {
        model: root.options

        QQC2.Button {
            required property var modelData
            required property int index

            text: modelData.label
            icon.name: modelData.icon || ""
            checkable: true
            checked: root.currentValue === modelData.value
            flat: !checked
            onClicked: root.chosen(modelData.value)

            QQC2.ToolTip.text: modelData.tooltip || ""
            QQC2.ToolTip.visible: hovered && (modelData.tooltip || "").length > 0
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
    }
}
