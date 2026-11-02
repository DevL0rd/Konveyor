import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/BorderActions.js" as BorderActions
import "../catalog/BorderStyles.js" as BorderStyles
import "../previews"
import org.kde.konveyor.settings

Kirigami.Dialog {
    id: chooser

    property string targetPath
    property string place
    readonly property var styles: visible ? SettingsStore.values["border-styles"] || [] : []
    readonly property var presetGroups: visible ? BorderStyles.groupedPresets() : []
    signal chosen(string name)

    function ask(path, where) {
        targetPath = path;
        place = where;
        open();
    }

    function finish(name) {
        close();
        if (name.length > 0) {
            chosen(name);
        }
    }

    component TileGrid: Item {
        default property alias tiles: grid.data
        readonly property real tileWidth: Math.floor((width - (grid.columns - 1) * grid.columnSpacing) / grid.columns)
        Layout.fillWidth: true
        implicitHeight: grid.implicitHeight

        GridLayout {
            id: grid
            anchors.left: parent.left
            anchors.top: parent.top
            columns: Math.max(1, Math.floor((parent.width + columnSpacing) / (150 + columnSpacing)))
            columnSpacing: Kirigami.Units.smallSpacing
            rowSpacing: Kirigami.Units.smallSpacing
        }
    }

    parent: QQC2.Overlay.overlay
    title: "Choose an animated style"
    standardButtons: Kirigami.Dialog.Cancel
    preferredWidth: Kirigami.Units.gridUnit * 46
    preferredHeight: Kirigami.Units.gridUnit * 34
    padding: Kirigami.Units.largeSpacing

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            text: chooser.place
            opacity: 0.75
            font: Kirigami.Theme.smallFont
        }

        QQC2.Label {
            visible: chooser.styles.length > 0
            text: "Your styles"
            font.weight: Font.Medium
        }

        TileGrid {
            id: ownGrid

            Repeater {
                model: chooser.styles

                StyleTile {
                    required property var modelData
                    Layout.preferredWidth: ownGrid.tileWidth
                    Layout.fillHeight: true
                    layers: modelData.layers
                    name: modelData.name
                    onActivated: chooser.finish(modelData.name)
                }
            }
        }

        QQC2.Label {
            Layout.topMargin: Kirigami.Units.smallSpacing
            text: "Starting points"
            font.weight: Font.Medium
        }

        Repeater {
            model: chooser.presetGroups

            ColumnLayout {
                id: presetGroup
                required property var modelData
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    text: presetGroup.modelData.label
                    font: Kirigami.Theme.smallFont
                    opacity: 0.75
                }

                TileGrid {
                    id: startGrid

                    Repeater {
                        model: presetGroup.modelData.presets

                        StyleTile {
                            required property var modelData
                            Layout.preferredWidth: startGrid.tileWidth
                            Layout.fillHeight: true
                            layers: BorderStyles.presetValues(SettingsStore, modelData)
                            name: modelData.label
                            tooltip: modelData.tooltip
                            onActivated: chooser.finish(BorderActions.ensurePreset(SettingsStore, modelData))
                        }
                    }
                }
            }
        }
    }
}
