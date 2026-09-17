import QtQuick
import ".."
import org.kde.konveyor.settings

SettingRow {
    id: row

    property string path
    property string yesLabel: "Yes"
    property string noLabel: "No"
    readonly property var node: SettingsStore.revision >= 0 ? SettingsStore.node(path) : ({})
    readonly property var current: node.args && node.args.length ? node.args[0] : null

    TriState {
        triState: row.current
        yesLabel: row.yesLabel
        noLabel: row.noLabel
        onStateChosen: state => {
            if (state === null) {
                SettingsStore.remove(row.path);
            } else {
                SettingsStore.setValue(row.path, [state]);
            }
        }
    }
}
