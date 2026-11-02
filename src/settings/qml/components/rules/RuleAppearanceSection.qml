import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import ".."
import org.kde.konveyor.settings

ColumnLayout {
    id: section

    property string rulePath
    readonly property var opacityNode: SettingsStore.revision >= 0 ? SettingsStore.node(rulePath + "/opacity") : ({})
    readonly property var radiusNode: SettingsStore.revision >= 0 ? SettingsStore.node(rulePath + "/geometry-corner-radius") : ({})
    readonly property var radii: radiusNode.args && radiusNode.args.length ? (radiusNode.args.length === 4 ? radiusNode.args : [radiusNode.args[0], radiusNode.args[0], radiusNode.args[0], radiusNode.args[0]]) : null

    spacing: 0

    SettingRow {
        label: "Opacity"
        description: "How see-through the window is"
        iconName: "view-visible"

        RowLayout {
            Segmented {
                currentValue: section.opacityNode.name !== undefined ? "custom" : "default"
                options: [
                    { value: "default", label: "Default" },
                    { value: "custom", label: "Custom" }
                ]
                onChosen: value => value === "default" ? SettingsStore.remove(section.rulePath + "/opacity") : SettingsStore.setValue(section.rulePath + "/opacity", [0.9])
            }

            QQC2.Slider {
                id: opacitySlider
                visible: section.opacityNode.name !== undefined
                from: 0.1
                to: 1
                stepSize: 0.01
                value: section.opacityNode.args && section.opacityNode.args.length ? section.opacityNode.args[0] : 1
                Layout.preferredWidth: Kirigami.Units.gridUnit * 9
                onPressedChanged: {
                    if (!pressed) {
                        SettingsStore.setValue(section.rulePath + "/opacity", [Number(value.toFixed(2))]);
                    }
                }
            }

            QQC2.Label {
                visible: opacitySlider.visible
                text: Math.round(opacitySlider.value * 100) + "%"
                Layout.preferredWidth: Kirigami.Units.gridUnit * 2.5
            }
        }
    }

    SettingRow {
        label: "Rounded corners"
        description: "Corner radius of the window, used for the focus ring and border too"
        iconName: "draw-rectangle"
        wideControl: true

        RuleCornersField {
            width: parent.width
            radii: section.radii
            onEdited: values => values === null
                ? SettingsStore.remove(section.rulePath + "/geometry-corner-radius")
                : SettingsStore.setValue(section.rulePath + "/geometry-corner-radius", values.every(value => value === values[0]) ? [values[0]] : values)
        }
    }

    RuleTriStateRow {
        path: section.rulePath + "/clip-to-geometry"
        label: "Clip the window to its corners"
        description: "Cut off square corners so apps without rounded corners match"
        iconName: "transform-crop"
        yesLabel: "Clip"
        noLabel: "Don't clip"
    }

    Kirigami.Separator {
        Layout.fillWidth: true
    }

    RuleBorderEditor {
        Layout.fillWidth: true
        blockPath: section.rulePath + "/focus-ring"
        label: "Custom focus ring"
        description: "Override the focus ring for these windows"
        iconName: "draw-circle"
    }

    RuleBorderEditor {
        Layout.fillWidth: true
        blockPath: section.rulePath + "/border"
        label: "Custom border"
        description: "Override the border for these windows"
        iconName: "draw-rectangle"
    }
}
