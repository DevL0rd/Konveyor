import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "LayoutKeys.js" as LayoutKeys
import org.kde.konveyor.settings

SettingRow {
    id: row

    required property var scope
    required property string key
    property var subPaths: []
    property bool showOverride: true
    property string inheritedSummary
    default property alias editor: holder.data
    readonly property string path: scope.scopePath + "/" + key
    readonly property bool overridden: SettingsStore.revision >= 0 && SettingsStore.has(path)
    readonly property bool editable: !scope.overrideMode || overridden

    visible: !scope.excluded.includes(key) && !(scope.overrideMode && !showOverride && !overridden)
    resetPaths: scope.overrideMode ? [] : (subPaths.length ? subPaths.map(sub => path + "/" + sub) : [path])

    RowLayout {
        width: row.wideControl ? parent.width : implicitWidth
        spacing: Kirigami.Units.largeSpacing

        ColumnLayout {
            Layout.fillWidth: row.wideControl
            spacing: Kirigami.Units.smallSpacing

            QQC2.CheckBox {
                visible: row.scope.overrideMode && row.showOverride
                text: row.overridden ? "Customized here" : "Customize here"
                checked: row.overridden
                onToggled: {
                    if (checked) {
                        LayoutKeys.write(SettingsStore, row.scope.scopePath, row.key, row.scope.values[row.key]);
                    } else {
                        SettingsStore.remove(row.path);
                    }
                }
            }

            QQC2.Label {
                visible: row.scope.overrideMode && !row.overridden && row.inheritedSummary.length > 0
                text: "Same as main layout: " + row.inheritedSummary
                opacity: 0.7
                wrapMode: Text.Wrap
                Layout.fillWidth: row.wideControl
            }

            Item {
                id: holder
                visible: !(row.scope.overrideMode && !row.overridden && row.inheritedSummary.length > 0)
                enabled: row.editable
                opacity: row.editable ? 1 : 0.45
                implicitWidth: row.wideControl ? 0 : childrenRect.width
                implicitHeight: childrenRect.height
                Layout.fillWidth: row.wideControl
            }
        }
    }
}
