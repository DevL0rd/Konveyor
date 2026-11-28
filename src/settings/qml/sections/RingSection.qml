import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import "LayoutKeys.js" as LayoutKeys
import org.kde.konveyor.settings

ColumnLayout {
    id: root

    property string scopePath: "layout"
    property bool overrideMode: false
    property var excluded: []
    property string key: "focus-ring"
    property string title
    property string summary
    property string iconName
    readonly property var values: SettingsStore.revision >= 0 ? SettingsStore.scope(scopePath) : ({})
    readonly property var ring: values[key] || ({ enabled: false, width: 4 })
    readonly property string blockPath: scopePath + "/" + key

    spacing: 0
    visible: !excluded.includes(key)

    CardHeader {
        title: root.title
    }

    Card {
        ScopedRow {
            scope: root
            key: root.key
            subPaths: ["on", "off"]
            label: "Show " + root.title.toLowerCase()
            description: root.summary
            iconName: root.iconName

            ScopedSwitch {
                isOn: root.ring.enabled === true
                onSwitched: on => SettingsStore.setToggle(root.blockPath, on)
            }
        }

        ScopedRow {
            scope: root
            key: root.key
            subPaths: ["width"]
            showOverride: false
            visible: root.ring.enabled === true && !root.excluded.includes(key) && (!root.overrideMode || overridden)
            label: "Thickness"
            iconName: "format-stroke-color"

            ValueSlider {
                value: root.ring.width || 0
                from: 1
                to: 24
                unit: "px"
                onEdited: value => SettingsStore.setValue(root.blockPath + "/width", [Math.round(value)])
            }
        }

        ScopedRow {
            scope: root
            key: "focus-ring-transition"
            wideControl: true
            visible: root.ring.enabled === true && !root.excluded.includes(root.key)
            inheritedSummary: ({ ride: "ride the motion", crossfade: "crossfade", delayed: "appear at the end" })[root.values["focus-ring-transition"]] || ""
            label: "Moving between workspaces"
            description: "What happens to the focused window's ring while the workspaces move. Only the ring changes; borders stay put."
            iconName: "transform-move"

            ChoiceCards {
                width: parent.width
                currentValue: root.values["focus-ring-transition"]
                cardHeight: Kirigami.Units.gridUnit * 7
                onChosen: value => LayoutKeys.write(SettingsStore, root.scopePath, "focus-ring-transition", value)
                options: [
                    { value: "ride", title: "Ride the motion", description: "Keeps each ring on its window as it moves" },
                    { value: "crossfade", title: "Crossfade", description: "Fades each ring with its window and brings it back if the switch is pulled back" },
                    { value: "delayed", title: "Delayed", description: "No ring until the switch settles, then the new window's ring appears" }
                ]
            }
        }

        Repeater {
            model: [
                { state: "active", label: "Focused window", description: "The window you're typing into", icon: "window-new" },
                { state: "inactive", label: "Inactive windows", description: "Every window that doesn't have focus", icon: "window" },
                { state: "urgent", label: "Needs attention", description: "A window asking for your attention", icon: "emblem-important" }
            ]

            ScopedRow {
                id: stateRow
                required property var modelData
                readonly property string stylePath: root.blockPath + "/" + modelData.state + "-style"
                readonly property string styleName: root.ring[modelData.state + "-style"] || ""
                readonly property bool animated: styleName.length > 0 && styleName !== "none"
                scope: root
                key: root.key
                subPaths: [modelData.state + "-color", modelData.state + "-gradient", modelData.state + "-style"]
                resetPaths: []
                showOverride: false
                visible: root.ring.enabled === true && !root.excluded.includes(key) && (!root.overrideMode || overridden)
                label: modelData.label
                description: modelData.description
                iconName: modelData.icon

                function chooseStyle() {
                    chooser.ask(stylePath, root.title + " · " + modelData.label);
                }

                function clearStyle() {
                    if (!root.overrideMode) {
                        SettingsStore.remove(stylePath);
                    } else if (animated) {
                        SettingsStore.setValue(stylePath, ["none"]);
                    }
                }

                PaintEditor {
                    blockPath: root.blockPath
                    colorName: modelData.state + "-color"
                    gradientName: modelData.state + "-gradient"
                    paint: root.ring[modelData.state]
                    allowAuto: root.key === "focus-ring" && modelData.state === "inactive" && !root.overrideMode
                    autoLabel: "Off"
                    extraModes: [{ value: "animated", label: "Animated" }]
                    extraMode: stateRow.animated ? "animated" : ""
                    resetPaths: root.overrideMode ? [] : stateRow.subPaths.map(sub => stateRow.path + "/" + sub)
                    stacked: root.width < Kirigami.Units.gridUnit * 40
                    onExtraChosen: stateRow.chooseStyle()
                    onPlainChosen: stateRow.clearStyle()

                    StyleSwatchButton {
                        visible: stateRow.animated
                        Layout.fillWidth: true
                        styleName: stateRow.styleName
                        onClicked: stateRow.chooseStyle()
                    }
                }
            }
        }
    }

    StyleChooser {
        id: chooser
        onChosen: name => SettingsStore.setValue(chooser.targetPath, [name])
    }
}
