import QtQuick
import QtQuick.Controls as QQC2

SettingRow {
    id: row

    property bool isOn
    signal switched(bool on)

    QQC2.Switch {
        checked: row.isOn
        onToggled: row.switched(checked)
        Accessible.name: row.label
    }
}
