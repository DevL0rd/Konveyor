import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import ".."
import org.kde.konveyor.settings

SettingRow {
    id: row

    property string path
    readonly property var node: SettingsStore.revision >= 0 ? SettingsStore.node(path) : ({})
    readonly property bool custom: node.args !== undefined && node.args.length > 0

    RowLayout {
        Segmented {
            currentValue: row.custom ? "custom" : "default"
            options: [
                { value: "default", label: "Default" },
                { value: "custom", label: "Custom" }
            ]
            onChosen: value => value === "default" ? SettingsStore.remove(row.path) : SettingsStore.setValue(row.path, [spin.value])
        }

        QQC2.SpinBox {
            id: spin
            visible: row.custom
            from: 1
            to: 64
            editable: true
            value: row.custom ? row.node.args[0] : 3
            onValueModified: SettingsStore.setValue(row.path, [value])
        }
    }
}
