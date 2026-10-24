import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.konveyor.settings

FormCard.AbstractFormDelegate {
    id: row

    property string label
    property string description
    property string iconName
    property var resetPaths: []
    default property alias control: controlSlot.data
    property bool wideControl: false
    property var taskbarKeys: []
    readonly property bool stacked: wideControl
        || controlSlot.implicitWidth + Kirigami.Units.gridUnit * 12 + 2 * Kirigami.Units.largeSpacing > availableWidth
    readonly property bool modified: (SettingsStore.revision >= 0 && resetPaths.some(path => !SettingsStore.isDefault(path)))
        || taskbarKeys.some(key => TaskbarSettings.values[key] !== undefined && !TaskbarSettings.isDefault(key))

    function reset() {
        for (const path of resetPaths) {
            SettingsStore.resetToDefault(path);
        }
        for (const key of taskbarKeys) {
            TaskbarSettings.reset(key);
        }
    }

    focusPolicy: Qt.NoFocus
    background: null

    contentItem: GridLayout {
        columns: row.stacked ? 1 : 3
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Icon {
                source: row.iconName
                visible: row.iconName.length > 0
                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        text: row.label
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    ModifiedBadge {
                        visible: row.modified
                        onResetRequested: row.reset()
                    }
                }

                QQC2.Label {
                    text: row.description
                    visible: text.length > 0
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    font: Kirigami.Theme.smallFont
                    Layout.fillWidth: true
                }
            }
        }

        Item {
            visible: !row.stacked
            Layout.preferredWidth: 0
        }

        Item {
            id: controlSlot
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
            Layout.alignment: row.stacked ? Qt.AlignLeft : (Qt.AlignRight | Qt.AlignVCenter)
            Layout.fillWidth: row.wideControl
        }
    }
}
