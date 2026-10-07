import QtQuick
import org.kde.konveyor.settings

SliderRow {
    id: row

    required property string key

    taskbarKeys: [key]
    unit: "px"
    value: Number(TaskbarSettings.values[key])
    onEdited: number => TaskbarSettings.values[row.key] = Math.round(number)
}
