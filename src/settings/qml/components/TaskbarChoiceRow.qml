import QtQuick
import org.kde.kirigami as Kirigami
import "../previews"
import org.kde.konveyor.settings

SettingRow {
    id: row

    required property string key
    property var options: []
    property var previewFor: value => {
        const overrides = {};
        overrides[row.key] = value;
        return overrides;
    }

    taskbarKeys: [key]
    wideControl: true

    ChoiceCards {
        width: parent.width
        cardHeight: Kirigami.Units.gridUnit * 6
        currentValue: TaskbarSettings.values[row.key]
        options: row.options.map(option => Object.assign({ preview: miniPreview }, option))
        onChosen: value => TaskbarSettings.values[row.key] = value
    }

    Component {
        id: miniPreview

        MiniTaskbar {
            settings: TaskbarSettings.values
            overrides: parent && parent.choice ? row.previewFor(parent.choice.value) : ({})
        }
    }
}
