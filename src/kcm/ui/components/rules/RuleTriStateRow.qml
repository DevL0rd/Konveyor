import QtQuick
import ".."

SettingRow {
    id: row

    property string path
    property string yesLabel: "Yes"
    property string noLabel: "No"
    readonly property var node: kcm.revision >= 0 ? kcm.node(path) : ({})
    readonly property var current: node.args && node.args.length ? node.args[0] : null

    TriState {
        triState: row.current
        yesLabel: row.yesLabel
        noLabel: row.noLabel
        onStateChosen: state => {
            if (state === null) {
                kcm.remove(row.path);
            } else {
                kcm.setValue(row.path, [state]);
            }
        }
    }
}
