import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"

ColumnLayout {
    id: root

    property string scopePath: "layout"
    property bool overrideMode: false
    property var excluded: []
    readonly property var values: kcm.revision >= 0 ? kcm.scope(scopePath) : ({})
    readonly property var tab: values["tab-indicator"] || ({})
    readonly property string blockPath: scopePath + "/tab-indicator"
    readonly property bool shown: tab.enabled === true

    function writeFlag(name, on) {
        if (on || root.overrideMode) {
            kcm.setValue(blockPath + "/" + name, on ? [] : [false]);
        } else {
            kcm.remove(blockPath + "/" + name);
        }
    }

    spacing: 0
    visible: !excluded.includes("tab-indicator")

    CardHeader {
        title: "Tab indicator"
    }

    Card {
        ScopedRow {
            scope: root
            key: "tab-indicator"
            subPaths: ["on", "off"]
            label: "Show tabs beside tabbed columns"
            description: "A strip of marks, one per window, so you can see how many tabs a column has and which one is showing."
            iconName: "tab-new"

            ScopedSwitch {
                isOn: root.shown
                onSwitched: on => kcm.setToggle(root.blockPath, on)
            }
        }

        ScopedRow {
            scope: root
            key: "tab-indicator"
            showOverride: false
            enabled: root.shown
            subPaths: ["position", "place-within-column"]
            label: "Placement"
            description: "Click a side of the column. Inside the column puts the marks over the window instead of in the gap."
            iconName: "align-vertical-center"

            ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                TabPositionPicker {
                    position: root.tab.position || "left"
                    inside: root.tab["place-within-column"] === true
                    onChosen: position => kcm.setValue(root.blockPath + "/position", [position])
                }

                QQC2.CheckBox {
                    text: "Inside the column"
                    checked: root.tab["place-within-column"] === true
                    onToggled: root.writeFlag("place-within-column", checked)
                }
            }
        }

        ScopedRow {
            scope: root
            key: "tab-indicator"
            showOverride: false
            enabled: root.shown
            subPaths: ["hide-when-single-tab"]
            label: "Hide when there's only one tab"
            iconName: "view-hidden"

            ScopedSwitch {
                isOn: root.tab["hide-when-single-tab"] === true
                onSwitched: on => root.writeFlag("hide-when-single-tab", on)
            }
        }

        BlockSliderRow {
            scope: root
            key: "tab-indicator"
            showOverride: false
            enabled: root.shown
            blockPath: root.blockPath
            name: "width"
            label: "Thickness"
            iconName: "format-stroke-color"
            current: root.tab.width || 0
            from: 1
        }

        BlockSliderRow {
            scope: root
            key: "tab-indicator"
            showOverride: false
            enabled: root.shown
            blockPath: root.blockPath
            name: "gap"
            label: "Distance from the window"
            iconName: "distribute-horizontal-x"
            current: root.tab.gap || 0
            from: -32
        }

        BlockSliderRow {
            scope: root
            key: "tab-indicator"
            showOverride: false
            enabled: root.shown
            blockPath: root.blockPath
            name: "length"
            propertyName: "total-proportion"
            label: "Length"
            description: "How much of the column's side the marks span together."
            iconName: "measure"
            current: root.tab.length || 0
            factor: 100
            from: 10
            to: 100
            unit: "%"
        }

        BlockSliderRow {
            scope: root
            key: "tab-indicator"
            showOverride: false
            enabled: root.shown
            blockPath: root.blockPath
            name: "gaps-between-tabs"
            label: "Space between tabs"
            iconName: "distribute-vertical-y"
            current: root.tab["gaps-between-tabs"] || 0
        }

        BlockSliderRow {
            scope: root
            key: "tab-indicator"
            showOverride: false
            enabled: root.shown
            blockPath: root.blockPath
            name: "corner-radius"
            label: "Roundness"
            iconName: "draw-rectangle"
            current: root.tab["corner-radius"] || 0
            to: 16
        }

        Repeater {
            model: [
                { state: "active", label: "Showing tab" },
                { state: "inactive", label: "Hidden tabs" },
                { state: "urgent", label: "Tab that needs attention" }
            ]

            ScopedRow {
                required property var modelData
                scope: root
                key: "tab-indicator"
                showOverride: false
                enabled: root.shown
                subPaths: [modelData.state + "-color", modelData.state + "-gradient"]
                label: modelData.label
                iconName: "color-management"

                PaintEditor {
                    blockPath: root.blockPath
                    colorName: modelData.state + "-color"
                    gradientName: modelData.state + "-gradient"
                    paint: root.tab[modelData.state]
                    allowAuto: true
                    autoLabel: "Match focus ring"
                }
            }
        }
    }
}
