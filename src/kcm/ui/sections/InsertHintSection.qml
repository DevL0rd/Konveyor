import QtQuick
import QtQuick.Layouts
import "../components"

ColumnLayout {
    id: root

    property string scopePath: "layout"
    property bool overrideMode: false
    property var excluded: []
    readonly property var values: kcm.revision >= 0 ? kcm.scope(scopePath) : ({})
    readonly property var hint: values["insert-hint"] || ({})
    readonly property string blockPath: scopePath + "/insert-hint"

    spacing: 0
    visible: !excluded.includes("insert-hint")

    CardHeader {
        title: "Drop hint"
    }

    Card {
        ScopedRow {
            scope: root
            key: "insert-hint"
            subPaths: ["on", "off"]
            label: "Show where a dragged window will land"
            description: "While you drag a window, a highlight marks the spot it will drop into."
            iconName: "transform-move"

            ScopedSwitch {
                isOn: root.hint.enabled === true
                onSwitched: on => kcm.setToggle(root.blockPath, on)
            }
        }

        ScopedRow {
            scope: root
            key: "insert-hint"
            subPaths: ["color", "gradient"]
            showOverride: false
            enabled: root.hint.enabled === true
            label: "Hint color"
            iconName: "color-management"

            PaintEditor {
                blockPath: root.blockPath
                colorName: "color"
                gradientName: "gradient"
                paint: root.hint.paint
            }
        }
    }
}
