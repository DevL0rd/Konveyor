import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import ".."
import "../../catalog/Kdl.js" as Kdl
import "RulePaint.js" as RulePaint

ColumnLayout {
    id: editor

    property string blockPath
    property string label
    property string description
    property string iconName
    readonly property var node: kcm.revision >= 0 ? kcm.node(blockPath) : ({})
    readonly property bool overridden: node.name !== undefined
    readonly property var blockChildren: node.children || []
    readonly property var enabledState: blockChildren.some(child => child.name === "on") ? true : (blockChildren.some(child => child.name === "off") ? false : null)
    readonly property var widthNode: blockChildren.find(child => child.name === "width")
    readonly property var states: [
        { key: "active", label: "Focused window" },
        { key: "inactive", label: "Other windows" },
        { key: "urgent", label: "Needs attention" }
    ]

    spacing: 0

    SwitchRow {
        label: editor.label
        description: editor.description
        iconName: editor.iconName
        isOn: editor.overridden
        onSwitched: on => on ? kcm.setNode(editor.blockPath, Kdl.block(editor.blockPath.split("/").pop(), [])) : kcm.remove(editor.blockPath)
    }

    SettingRow {
        visible: editor.overridden
        label: "Show it"

        TriState {
            unsetLabel: "Inherit"
            yesLabel: "On"
            noLabel: "Off"
            triState: editor.enabledState
            onStateChosen: state => {
                kcm.remove(editor.blockPath + "/on");
                kcm.remove(editor.blockPath + "/off");
                if (state !== null) {
                    kcm.append(editor.blockPath, Kdl.leaf(state ? "on" : "off"));
                }
            }
        }
    }

    SettingRow {
        visible: editor.overridden
        label: "Width"

        RowLayout {
            Segmented {
                currentValue: editor.widthNode ? "custom" : "inherit"
                options: [
                    { value: "inherit", label: "Inherit" },
                    { value: "custom", label: "Custom" }
                ]
                onChosen: value => value === "inherit" ? kcm.remove(editor.blockPath + "/width") : kcm.setValue(editor.blockPath + "/width", [widthSpin.value])
            }

            QQC2.SpinBox {
                id: widthSpin
                visible: editor.widthNode !== undefined
                from: 0
                to: 64
                editable: true
                value: editor.widthNode ? editor.widthNode.args[0] : 4
                textFromValue: (number, locale) => number + " px"
                valueFromText: (text, locale) => parseInt(text)
                onValueModified: kcm.setValue(editor.blockPath + "/width", [value])
            }
        }
    }

    Repeater {
        model: editor.overridden ? editor.states : []

        SettingRow {
            id: paintRow
            required property var modelData
            label: modelData.label

            PaintEditor {
                blockPath: editor.blockPath
                colorName: paintRow.modelData.key + "-color"
                gradientName: paintRow.modelData.key + "-gradient"
                paint: RulePaint.paintOf(editor.node, paintRow.modelData.key)
                allowAuto: true
                autoLabel: "Inherit"
            }
        }
    }
}
