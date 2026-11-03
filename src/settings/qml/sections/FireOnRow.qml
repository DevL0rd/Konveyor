import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../catalog/Signals.js" as Signals
import org.kde.konveyor.settings

SettingRow {
    id: row

    property string layerPath
    property var layerValues: null
    readonly property var chosen: (layerValues || {}).fireOn || []

    function toggle(signal) {
        const next = chosen.includes(signal) ? chosen.filter(entry => entry !== signal) : chosen.concat([signal]);
        if (next.length) {
            SettingsStore.setValue(layerPath + "/fire-on", next);
        } else {
            SettingsStore.remove(layerPath + "/fire-on");
        }
    }

    label: "Fire on"
    description: "What starts it. Without any, it runs on its own."
    resetPaths: [layerPath + "/fire-on"]
    wideControl: true

    Flow {
        width: parent.width
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            model: Signals.impulses()

            QQC2.Button {
                required property var modelData
                text: modelData.label
                checkable: true
                checked: row.chosen.includes(modelData.value)
                onClicked: row.toggle(modelData.value)
            }
        }
    }
}
