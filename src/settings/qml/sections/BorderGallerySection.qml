import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Shapes
import org.kde.kirigami as Kirigami
import "../components"
import "../components/monitors"
import "../previews"
import "../catalog/BorderActions.js" as BorderActions
import "../catalog/BorderStyles.js" as BorderStyles
import "../catalog/Kdl.js" as Kdl
import org.kde.konveyor.settings

ColumnLayout {
    id: root

    property Item tryOnTile: null
    readonly property var tryOn: tryOnTile ? { label: tryOnTile.name, layers: tryOnTile.layers } : null
    readonly property var values: SettingsStore.revision >= 0 ? SettingsStore.scope("layout") : ({})
    readonly property var entries: SettingsStore.revision >= 0 ? SettingsStore.children("", "border-style") : []
    readonly property var parsed: SettingsStore.values["border-styles"] || []
    readonly property var ownNames: entries.map(BorderStyles.nameOf)
    readonly property var included: parsed.filter(style => !ownNames.includes(style.name))
    readonly property var namesByLook: BorderActions.namesByLook(parsed)
    readonly property var taken: SettingsStore.revision >= 0 ? BorderActions.takenNames(SettingsStore) : []
    readonly property string partWord: BorderStyles.partLabel(BorderStyles.targetPart(values))
    property string renamingPath
    property string renamingName

    function followTile(tile) {
        if (tile.tryingOn) {
            tryOnTile = tile;
        } else if (tryOnTile === tile) {
            tryOnTile = null;
        }
    }

    function openStyle(path) {
        SettingsStore.save();
        if (path.length > 0 && SettingsStore.has(path)) {
            tryOnTile = null;
            SettingsNavigation.push("pages/BorderStyleEditorPage.qml", { stylePath: path });
        }
    }

    function usageOf(name) {
        return BorderStyles.usage(values["focus-ring"], values.border, name);
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

    component UseButton: Item {
        id: useButton
        property string styleName
        readonly property bool inUse: BorderActions.inUse(root.values, styleName)
        readonly property string label: BorderActions.useLabel(root.values, styleName)
        signal used()
        Layout.fillWidth: true
        implicitHeight: useAction.implicitHeight

        QQC2.Button {
            id: useAction
            anchors.fill: parent
            visible: !useButton.inUse
            text: useButton.label
            icon.name: "dialog-ok-apply"
            onClicked: useButton.used()

            QQC2.ToolTip.text: BorderActions.useTip(root.values)
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        RowLayout {
            anchors.fill: parent
            visible: useButton.inUse
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Icon {
                Layout.leftMargin: Kirigami.Units.smallSpacing
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: implicitWidth
                source: "dialog-ok-apply"
            }

            QQC2.Label {
                Layout.fillWidth: true
                text: useButton.label
                elide: Text.ElideRight
            }
        }
    }

    component TileAction: QQC2.ToolButton {
        display: QQC2.AbstractButton.IconOnly
        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    }

    spacing: 0

    NamePrompt {
        id: newStyle
        parent: QQC2.Overlay.overlay
        title: "New border style"
        label: "Style name"
        taken: root.taken
        onNamed: name => root.openStyle(SettingsStore.append("", Kdl.block("border-style", [BorderStyles.newLayer("gradient")], [name])))
    }

    NamePrompt {
        id: rename
        parent: QQC2.Overlay.overlay
        title: "Rename style"
        label: "Style name"
        taken: root.taken
        onNamed: name => BorderStyles.rename(SettingsStore, root.renamingPath, root.renamingName, name)
    }

    CardHeader {
        title: "Animated styles"
    }

    Card {
        QQC2.Label {
            text: "A style is a stack of effects you can mix. Hover one to try it on the " + root.partWord + " above, click it to edit it."
            wrapMode: Text.Wrap
            opacity: 0.75
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            Layout.bottomMargin: 0
        }

        QQC2.Label {
            text: "Your styles"
            font.weight: Font.Medium
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
        }

        TileGrid {
            id: mineGrid
            Layout.margins: Kirigami.Units.largeSpacing

            Repeater {
                model: root.entries.length

                StyleTile {
                    id: ownTile
                    required property int index
                    readonly property var entry: root.entries[index] || ({ node: {}, path: "" })
                    Layout.preferredWidth: mineGrid.tileWidth
                    Layout.fillHeight: true
                    name: BorderStyles.nameOf(entry)
                    layers: BorderStyles.layersNamed(root.parsed, name)
                    usage: root.usageOf(name)
                    onTryingOnChanged: root.followTile(ownTile)
                    onActivated: root.openStyle(entry.path)

                    UseButton {
                        styleName: ownTile.name
                        onUsed: BorderActions.use(SettingsStore, root.values, ownTile.name)
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        TileAction {
                            icon.name: "edit-rename"
                            text: "Rename"
                            onClicked: {
                                root.renamingPath = ownTile.entry.path;
                                root.renamingName = ownTile.name;
                                rename.ask(ownTile.name);
                            }
                        }

                        TileAction {
                            icon.name: "edit-copy"
                            text: "Duplicate"
                            onClicked: SettingsStore.append("", BorderActions.duplicateNode(SettingsStore, ownTile.entry, ownTile.name))
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        TileAction {
                            icon.name: "edit-delete"
                            text: "Delete"
                            onClicked: BorderStyles.remove(SettingsStore, ownTile.entry.path, ownTile.name)
                        }
                    }
                }
            }

            Repeater {
                model: root.included

                StyleTile {
                    id: includedTile
                    required property var modelData
                    Layout.preferredWidth: mineGrid.tileWidth
                    Layout.fillHeight: true
                    name: modelData.name
                    layers: modelData.layers
                    usage: root.usageOf(name)
                    locked: true
                    interactive: false
                    tooltip: "From an included file"
                    onTryingOnChanged: root.followTile(includedTile)

                    UseButton {
                        styleName: includedTile.name
                        onUsed: BorderActions.use(SettingsStore, root.values, includedTile.name)
                    }

                    TileAction {
                        icon.name: "edit-copy"
                        text: "Duplicate as your own"
                        onClicked: SettingsStore.append("", BorderActions.duplicateNode(SettingsStore, null, includedTile.name))
                    }
                }
            }

            QQC2.AbstractButton {
                id: newTile
                Layout.preferredWidth: mineGrid.tileWidth
                Layout.fillHeight: true
                Layout.minimumHeight: 148
                text: "New style"
                hoverEnabled: true
                onClicked: newStyle.ask("")

                background: Item {
                    Shape {
                        anchors.fill: parent

                        ShapePath {
                            strokeColor: newTile.hovered || newTile.visualFocus ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.4)
                            strokeWidth: 1
                            strokeStyle: ShapePath.DashLine
                            dashPattern: [4, 3]
                            fillColor: "transparent"

                            PathRectangle {
                                x: 0.5
                                y: 0.5
                                width: newTile.width - 1
                                height: newTile.height - 1
                                radius: 5
                            }
                        }
                    }
                }

                contentItem: Item {
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            Layout.alignment: Qt.AlignHCenter
                            implicitWidth: Kirigami.Units.iconSizes.medium
                            implicitHeight: implicitWidth
                            source: "list-add"
                        }

                        QQC2.Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: newTile.text
                        }
                    }
                }
            }
        }

        QQC2.Label {
            text: "Starting points"
            font.weight: Font.Medium
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
        }

        QQC2.Label {
            text: "Click one to make your own copy and edit it."
            font: Kirigami.Theme.smallFont
            opacity: 0.75
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
        }

        Repeater {
            model: BorderStyles.groupedPresets()

            ColumnLayout {
                id: presetGroup
                required property var modelData
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
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
                            id: presetTile
                            required property var modelData
                            readonly property string look: JSON.stringify(layers)
                            Layout.preferredWidth: startGrid.tileWidth
                            Layout.fillHeight: true
                            name: modelData.label
                            layers: BorderStyles.presetValues(SettingsStore, modelData)
                            tooltip: modelData.tooltip
                            onTryingOnChanged: root.followTile(presetTile)
                            onActivated: root.openStyle(BorderActions.addCopy(SettingsStore, modelData))

                            UseButton {
                                styleName: root.namesByLook[presetTile.look] || ""
                                onUsed: BorderActions.use(SettingsStore, root.values, BorderActions.ensurePreset(SettingsStore, presetTile.modelData))
                            }
                        }
                    }
                }
            }
        }

        SwitchRow {
            label: "Reduce motion"
            description: "Hold every animated style still. Previews follow."
            iconName: "preferences-desktop-effects"
            isOn: SettingsStore.values["animations/reduce-motion"] === true
            resetPaths: ["animations/reduce-motion"]
            onSwitched: on => SettingsStore.setFlag("animations/reduce-motion", on)
        }
    }
}
