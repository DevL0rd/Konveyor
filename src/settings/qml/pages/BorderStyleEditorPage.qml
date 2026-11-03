import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../components/monitors"
import "../previews"
import "../sections"
import "../catalog/BorderActions.js" as BorderActions
import "../catalog/BorderStyles.js" as BorderStyles
import "../catalog/Kdl.js" as Kdl
import "../catalog/Signals.js" as Signals
import org.kde.konveyor.settings

SettingsPage {
    id: page

    property string stylePath
    property int openLayer: -1
    property int emphasizedLayer: -1
    readonly property string subtitle: "Animated style"
    readonly property var styleNode: SettingsStore.revision >= 0 ? SettingsStore.node(stylePath) : ({})
    readonly property bool exists: styleNode.name !== undefined
    readonly property string styleName: exists && styleNode.args.length ? String(styleNode.args[0]) : ""
    readonly property var layerEntries: SettingsStore.revision >= 0 && exists ? SettingsStore.children(stylePath, "layer") : []
    readonly property var parsedStyles: SettingsStore.values["border-styles"] || []
    readonly property var parsed: parsedStyles.find(style => style.name === styleName) || ({ layers: [] })
    readonly property var scoped: SettingsStore.revision >= 0 ? SettingsStore.scope("layout") : ({})
    readonly property var names: parsedStyles.map(style => style.name).concat(SettingsStore.revision >= 0 ? SettingsStore.children("", "border-style").map(BorderStyles.nameOf) : [], ["none"])
    readonly property string usage: BorderStyles.usage(scoped["focus-ring"], scoped.border, styleName)
    readonly property bool inUseOnTarget: BorderActions.inUse(scoped, styleName)

    function layerPath(index) {
        return stylePath + "/layer#" + index;
    }

    function moveLayer(index, delta) {
        if (!SettingsStore.move(layerPath(index), delta).length) {
            return;
        }
        if (openLayer === index) {
            openLayer = index + delta;
        } else if (openLayer === index + delta) {
            openLayer = index;
        }
    }

    function addLayer(kind) {
        if (SettingsStore.append(stylePath, BorderStyles.newLayer(kind)).length) {
            emphasizedLayer = -1;
            openLayer = layerEntries.length - 1;
        }
    }

    function removeLayer(index) {
        if (!SettingsStore.remove(layerPath(index))) {
            return;
        }
        emphasizedLayer = -1;
        if (openLayer >= index) {
            openLayer = Math.max(0, openLayer - 1);
        }
    }

    Component.onCompleted: openLayer = layerEntries.length - 1

    component KindTile: QQC2.AbstractButton {
        id: tile

        required property var modelData

        width: parent ? parent.tileWidth : 0
        hoverEnabled: true
        padding: 1
        text: modelData.label
        onClicked: page.addLayer(modelData.value)
        QQC2.ToolTip.text: "Add " + modelData.label + ". " + modelData.tooltip + "\n" + Signals.tooltipFor(BorderStyles.kindValues(SettingsStore, modelData.value))
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

        background: Rectangle {
            radius: 5
            color: Kirigami.Theme.backgroundColor
            border.width: 1
            border.color: tile.hovered || tile.visualFocus ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.2)
        }

        contentItem: ColumnLayout {
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * 4
                clip: true

                StyleBackdrop {
                    anchors.fill: parent
                    topRadius: 4
                }

                MockWindow {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.gridUnit * 0.8
                    zoom: 1
                    barHeight: 0
                    state_: "active"
                    cornerRadius: 4
                    ring: ({ enabled: false, width: page.scoped["focus-ring"] ? page.scoped["focus-ring"].width : 3 })
                    overridePart: "focus-ring"
                    overrideLayers: BorderStyles.kindValues(SettingsStore, tile.modelData.value)
                    rippleRest: tile.modelData.value === "ripple" ? 1.4 : 3
                }

                SignalBadge {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: 2
                    showTips: false
                    layers: BorderStyles.kindValues(SettingsStore, tile.modelData.value)
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.smallSpacing
                Layout.bottomMargin: Kirigami.Units.smallSpacing
                Layout.maximumWidth: tile.availableWidth - 4
                spacing: 2

                Kirigami.Icon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.small
                    Layout.preferredHeight: Kirigami.Units.iconSizes.small
                    source: "list-add-symbolic"
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: tile.modelData.label
                    elide: Text.ElideRight
                }
            }
        }
    }

    title: exists ? "Style: " + styleName : "Border style"
    previewBackdrop: true
    preview: BorderStylePreview {
        layers: page.parsed.layers
        caption: page.styleName
        baseWidth: page.scoped["focus-ring"] ? page.scoped["focus-ring"].width : 4
        emphasis: page.emphasizedLayer
    }

    NamePrompt {
        id: rename
        title: "Rename style"
        label: "Style name"
        taken: page.names
        onNamed: name => BorderStyles.rename(SettingsStore, page.stylePath, page.styleName, name)
    }

    Kirigami.PlaceholderMessage {
        visible: !page.exists
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.gridUnit * 2
        icon.name: "dialog-question"
        text: "This style no longer exists"
    }

    Card {
        visible: page.exists
        Layout.topMargin: Kirigami.Units.gridUnit

        GridLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            columns: page.width < Kirigami.Units.gridUnit * 44 ? 1 : 2
            columnSpacing: Kirigami.Units.smallSpacing
            rowSpacing: Kirigami.Units.smallSpacing

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                Kirigami.Heading {
                    level: 3
                    text: page.styleName
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                QQC2.Label {
                    text: page.usage.length ? page.usage : "Not in use yet"
                    wrapMode: Text.Wrap
                    opacity: 0.75
                    Layout.fillWidth: true
                }
            }

            RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.Button {
                    visible: !page.inUseOnTarget
                    icon.name: "dialog-ok-apply"
                    text: BorderActions.useLabel(page.scoped, page.styleName)
                    onClicked: BorderActions.use(SettingsStore, page.scoped, page.styleName)
                    QQC2.ToolTip.text: BorderActions.useTip(page.scoped)
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }

                Kirigami.Icon {
                    visible: page.inUseOnTarget
                    source: "dialog-ok-apply"
                    Layout.preferredWidth: Kirigami.Units.iconSizes.small
                    Layout.preferredHeight: Kirigami.Units.iconSizes.small
                }

                QQC2.Label {
                    visible: page.inUseOnTarget
                    text: BorderActions.useLabel(page.scoped, page.styleName)
                }

                QQC2.ToolButton {
                    icon.name: "edit-rename"
                    display: QQC2.AbstractButton.IconOnly
                    text: "Rename"
                    onClicked: rename.ask(page.styleName)
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                }

                QQC2.ToolButton {
                    icon.name: "edit-copy"
                    display: QQC2.AbstractButton.IconOnly
                    text: "Duplicate"
                    onClicked: {
                        const name = BorderStyles.uniqueName(page.styleName, page.names);
                        const path = SettingsStore.append("", Kdl.block("border-style", page.styleNode.children || [], [name]));
                        if (path.length) {
                            SettingsNavigation.push("pages/BorderStyleEditorPage.qml", { stylePath: path });
                        }
                    }
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                }

                QQC2.ToolButton {
                    icon.name: "edit-delete"
                    display: QQC2.AbstractButton.IconOnly
                    text: "Delete"
                    onClicked: {
                        if (BorderStyles.remove(SettingsStore, page.stylePath, page.styleName)) {
                            SettingsNavigation.pop();
                        }
                    }
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                }
            }
        }
    }

    CardHeader {
        visible: page.exists
        title: "Add a layer"
    }

    Card {
        visible: page.exists

        Loader {
            asynchronous: true
            Layout.fillWidth: true

            sourceComponent: ColumnLayout {
                spacing: 0

                Repeater {
                    model: BorderStyles.groupedKinds()

                    ColumnLayout {
                        id: kindGroup
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.margins: Kirigami.Units.largeSpacing
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            text: kindGroup.modelData.label
                            font: Kirigami.Theme.smallFont
                            opacity: 0.75
                        }

                        Flow {
                            readonly property int columns: Math.max(1, Math.floor((width + spacing) / (Kirigami.Units.gridUnit * 5 + spacing)))
                            readonly property real tileWidth: Math.floor((width - (columns - 1) * spacing) / columns)
                            Layout.fillWidth: true
                            spacing: Kirigami.Units.smallSpacing

                            Repeater {
                                model: kindGroup.modelData.kinds

                                KindTile {}
                            }
                        }
                    }
                }
            }
        }
    }

    CardHeader {
        visible: page.exists
        title: "Layers"
    }

    Card {
        visible: page.exists

        QQC2.Label {
            text: page.layerEntries.length
                ? "Layers on top draw over the ones below. Hover a layer to see which part of the look it makes."
                : "No layers yet. Add one above."
            wrapMode: Text.Wrap
            opacity: 0.75
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
        }
    }

    Repeater {
        model: page.layerEntries.length

        BorderLayerCard {
            visible: page.exists
            total: page.layerEntries.length
            layerPath: page.layerPath(layerIndex)
            node: page.layerEntries[layerIndex] ? page.layerEntries[layerIndex].node : ({})
            layerValues: page.parsed.layers[layerIndex] || null
            baseWidth: page.scoped["focus-ring"] ? page.scoped["focus-ring"].width : 3
            expanded: page.openLayer === layerIndex
            onToggled: page.openLayer = expanded ? -1 : layerIndex
            onMovedUp: page.moveLayer(layerIndex, 1)
            onMovedDown: page.moveLayer(layerIndex, -1)
            onRemoved: page.removeLayer(layerIndex)
            onPointingChanged: {
                if (pointing) {
                    page.emphasizedLayer = layerIndex;
                } else if (page.emphasizedLayer === layerIndex) {
                    page.emphasizedLayer = -1;
                }
            }
        }
    }
}
