import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import ".."
import org.kde.konveyor.settings

SettingRow {
    id: row

    property string path
    readonly property var node: SettingsStore.revision >= 0 ? SettingsStore.node(path) : ({})
    readonly property string current: node.args && node.args.length ? node.args[0] : "default"

    wideControl: true

    ChoiceCards {
        width: parent.width
        cardHeight: Kirigami.Units.gridUnit * 5.5
        currentValue: row.current
        options: [
            { value: "default", title: "Not pinned", description: "Opens next to the focused column", preview: rowDiagram },
            { value: "start", title: "Start of the row", description: "Always the first column", preview: rowDiagram },
            { value: "end", title: "End of the row", description: "Always the last column", preview: rowDiagram }
        ]
        onChosen: value => value === "default" ? SettingsStore.remove(row.path) : SettingsStore.setValue(row.path, [value])
    }

    Component {
        id: rowDiagram

        Row {
            id: diagram
            readonly property string pin: parent ? parent.choice.value : "default"
            readonly property bool isSelected: parent ? parent.selected : false
            spacing: 3

            Repeater {
                model: 5

                Rectangle {
                    required property int index
                    readonly property bool pinned: (diagram.pin === "start" && index === 0) || (diagram.pin === "end" && index === 4) || (diagram.pin === "default" && index === 2)
                    width: (diagram.width - 12) / 5
                    height: diagram.height
                    radius: 3
                    color: pinned ? Qt.alpha(Kirigami.Theme.highlightColor, diagram.isSelected ? 0.9 : 0.55) : Qt.alpha(Kirigami.Theme.textColor, 0.12)
                }
            }
        }
    }
}
