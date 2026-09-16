import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import "LayoutKeys.js" as LayoutKeys

ColumnLayout {
    id: root

    property string scopePath: "layout"
    property bool overrideMode: false
    property var excluded: []
    property string key: "focus-ring"
    property string title
    property string summary
    property string iconName
    readonly property var values: kcm.revision >= 0 ? kcm.scope(scopePath) : ({})
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
                onSwitched: on => kcm.setToggle(root.blockPath, on)
            }
        }

        ScopedRow {
            scope: root
            key: root.key
            subPaths: ["width"]
            showOverride: false
            enabled: root.ring.enabled === true
            label: "Thickness"
            iconName: "format-stroke-color"

            ValueSlider {
                value: root.ring.width || 0
                from: 1
                to: 24
                unit: "px"
                onEdited: value => kcm.setValue(root.blockPath + "/width", [Math.round(value)])
            }
        }

        Repeater {
            model: [
                { state: "active", label: "Focused window", description: "The window you're typing into", icon: "window-new" },
                { state: "inactive", label: "Other windows", description: "Every window that doesn't have focus", icon: "window" },
                { state: "urgent", label: "Needs attention", description: "A window asking for your attention", icon: "emblem-important" }
            ]

            ScopedRow {
                required property var modelData
                scope: root
                key: root.key
                subPaths: [modelData.state + "-color", modelData.state + "-gradient"]
                showOverride: false
                enabled: root.ring.enabled === true
                label: modelData.label
                description: modelData.description
                iconName: modelData.icon

                PaintEditor {
                    blockPath: root.blockPath
                    colorName: modelData.state + "-color"
                    gradientName: modelData.state + "-gradient"
                    paint: root.ring[modelData.state]
                }
            }
        }
    }
}
