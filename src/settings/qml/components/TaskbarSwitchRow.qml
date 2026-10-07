import QtQuick
import org.kde.konveyor.settings

SwitchRow {
    id: row

    required property string key

    taskbarKeys: [key]
    isOn: TaskbarSettings.values[key] === true
    onSwitched: on => TaskbarSettings.values[row.key] = on
}
