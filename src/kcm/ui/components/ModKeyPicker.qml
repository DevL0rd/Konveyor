import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

RowLayout {
    id: picker

    property string value
    signal picked(string name)

    readonly property var keys: [
        { name: "Super", label: "Meta", hint: "The Windows or Command key" },
        { name: "Alt", label: "Alt", hint: "Can clash with app menus" },
        { name: "Ctrl", label: "Ctrl", hint: "Clashes with most app shortcuts" },
        { name: "Shift", label: "Shift", hint: "Clashes with typing" },
        { name: "ISO_Level3_Shift", label: "AltGr", hint: "Right Alt on many layouts" },
        { name: "ISO_Level5_Shift", label: "Level 5", hint: "Only on layouts that have it" }
    ]

    spacing: Kirigami.Units.smallSpacing

    Repeater {
        model: picker.keys

        QQC2.Button {
            required property var modelData
            readonly property bool selected: picker.value === modelData.name

            text: modelData.label
            checkable: true
            checked: selected
            font.bold: selected
            onClicked: picker.picked(modelData.name)

            QQC2.ToolTip.text: modelData.hint
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
    }
}
