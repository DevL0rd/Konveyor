import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import "LayoutKeys.js" as LayoutKeys
import "../catalog/Kdl.js" as Kdl

ColumnLayout {
    id: root

    property string scopePath: "layout"
    property bool overrideMode: false
    property var excluded: []
    readonly property var values: kcm.revision >= 0 ? kcm.scope(scopePath) : ({})
    readonly property var output: kcm.live.outputs.length > 0 ? kcm.live.outputs[0].logical : ({ width: 1920, height: 1080 })

    spacing: 0

    CardHeader {
        title: "Spacing and sizes"
    }

    Card {
        ScopedRow {
            scope: root
            key: "gaps"
            label: "Gaps"
            description: "Space between columns, between stacked windows and around the screen edge."
            iconName: "distribute-horizontal-x"

            ValueSlider {
                value: root.values.gaps || 0
                to: 64
                unit: "px"
                onEdited: value => LayoutKeys.write(kcm, root.scopePath, "gaps", Math.round(value))
            }
        }

        ScopedRow {
            scope: root
            key: "preset-column-widths"
            inheritedSummary: (root.values["preset-column-widths"] || []).map(Kdl.sizeLabel).join("  ·  ")
            wideControl: true
            label: "Width presets"
            description: "The sizes “switch preset column width” cycles through. Drag the handle to resize a preset."
            iconName: "zoom-fit-width"

            PresetSizeBars {
                width: parent.width
                sizes: root.values["preset-column-widths"] || []
                screenLength: root.output.width
                addLabel: "Add a width"
                onEdited: sizes => LayoutKeys.write(kcm, root.scopePath, "preset-column-widths", sizes)
            }
        }

        ScopedRow {
            scope: root
            key: "preset-window-heights"
            inheritedSummary: (root.values["preset-window-heights"] || []).map(Kdl.sizeLabel).join("  ·  ")
            wideControl: true
            label: "Height presets"
            description: "The sizes “switch preset window height” cycles through, as a share of the screen height."
            iconName: "zoom-fit-height"

            PresetSizeBars {
                width: parent.width
                sizes: root.values["preset-window-heights"] || []
                screenLength: root.output.height
                addLabel: "Add a height"
                onEdited: sizes => LayoutKeys.write(kcm, root.scopePath, "preset-window-heights", sizes)
            }
        }
    }

    CardHeader {
        title: "Screen"
    }

    Card {
        ScopedRow {
            scope: root
            key: "struts"
            inheritedSummary: ["left", "right", "top", "bottom"].map(side => side + " " + Math.round((root.values.struts || {})[side] || 0) + " px").join("  ·  ")
            wideControl: true
            label: "Reserved screen edges"
            description: "Keep columns away from an edge, for example to leave room for a dock or a widget. Negative values let windows extend under it."
            iconName: "transform-crop"

            StrutsEditor {
                width: parent.width
                struts: root.values.struts || ({ left: 0, right: 0, top: 0, bottom: 0 })
                screenWidth: root.output.width
                screenHeight: root.output.height
                onEdited: struts => LayoutKeys.write(kcm, root.scopePath, "struts", struts)
            }
        }

        ScopedRow {
            scope: root
            key: "background-color"
            label: "Workspace background"
            description: "Shown behind the columns where no window covers the screen."
            iconName: "fill-color"

            ColorField {
                value: root.values["background-color"] || ""
                onEdited: css => LayoutKeys.write(kcm, root.scopePath, "background-color", css)
            }
        }
    }
}
