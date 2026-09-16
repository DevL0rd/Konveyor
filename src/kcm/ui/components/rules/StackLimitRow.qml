import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import ".."

SettingRow {
    id: row

    property string path
    readonly property var node: kcm.revision >= 0 ? kcm.node(path) : ({})
    readonly property bool custom: node.args !== undefined && node.args.length > 0

    RowLayout {
        Segmented {
            currentValue: row.custom ? "custom" : "default"
            options: [
                { value: "default", label: "Default" },
                { value: "custom", label: "Custom" }
            ]
            onChosen: value => value === "default" ? kcm.remove(row.path) : kcm.setValue(row.path, [spin.value])
        }

        QQC2.SpinBox {
            id: spin
            visible: row.custom
            from: 1
            to: 64
            editable: true
            value: row.custom ? row.node.args[0] : 3
            onValueModified: kcm.setValue(row.path, [value])
        }
    }
}
