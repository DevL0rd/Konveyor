import QtQuick
import QtQuick.Layouts
import ".."
import "../../catalog/Kdl.js" as Kdl

SettingRow {
    id: row

    property string path
    property string nodeName
    property string presetsKey: "preset-column-widths"
    readonly property var node: kcm.revision >= 0 ? kcm.node(path) : ({})
    readonly property bool present: node.name !== undefined
    readonly property var inner: present && node.children && node.children.length ? node.children[0] : null
    readonly property string mode: !present ? "default" : (inner ? "size" : "auto")
    readonly property var size: inner ? { kind: inner.name, value: inner.args[0] } : null

    wideControl: true

    RowLayout {
        width: parent.width

        Segmented {
            currentValue: row.mode
            options: [
                { value: "default", label: "Default" },
                { value: "auto", label: "App decides" },
                { value: "size", label: "Set a size" }
            ]
            onChosen: value => {
                if (value === "default") {
                    kcm.remove(row.path);
                } else if (value === "auto") {
                    kcm.setNode(row.path, Kdl.block(row.nodeName, []));
                } else {
                    kcm.setNode(row.path, Kdl.sizeBlock(row.nodeName, row.size || { kind: "proportion", value: 0.5 }));
                }
            }
        }

        SizePicker {
            visible: row.mode === "size"
            allowAuto: false
            size: row.size
            presets: kcm.revision >= 0 ? (kcm.scope("layout")[row.presetsKey] || []) : []
            onEdited: size => kcm.setNode(row.path, Kdl.sizeBlock(row.nodeName, size))
        }
    }
}
