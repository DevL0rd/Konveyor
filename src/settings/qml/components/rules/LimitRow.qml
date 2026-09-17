import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import ".."
import org.kde.konveyor.settings

SettingRow {
    id: row

    property string path
    readonly property var node: SettingsStore.revision >= 0 ? SettingsStore.node(path) : ({})
    readonly property bool limited: node.args !== undefined && node.args.length > 0

    RowLayout {
        Segmented {
            currentValue: row.limited ? "limit" : "none"
            options: [
                { value: "none", label: "No limit" },
                { value: "limit", label: "Limit" }
            ]
            onChosen: value => value === "none" ? SettingsStore.remove(row.path) : SettingsStore.setValue(row.path, [spin.value])
        }

        QQC2.SpinBox {
            id: spin
            visible: row.limited
            from: 1
            to: 16000
            stepSize: 50
            editable: true
            value: row.limited ? row.node.args[0] : 800
            textFromValue: (number, locale) => number + " px"
            valueFromText: (text, locale) => parseInt(text)
            onValueModified: SettingsStore.setValue(row.path, [value])
        }
    }
}
