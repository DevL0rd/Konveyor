import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import "../catalog/BorderStyles.js" as BorderStyles
import org.kde.konveyor.settings

Card {
    id: card

    property string layerPath
    property var layerValues: null
    property var node: ({})
    required property int index
    property int total
    property bool expanded
    property real baseWidth: 3
    signal toggled
    signal movedUp
    signal movedDown
    signal removed

    readonly property int layerIndex: total - 1 - index
    readonly property string kind: (node.args || [])[0] || "gradient"
    readonly property var kindInfo: BorderStyles.kindOf(kind)
    readonly property var rawColors: ((node.children || []).find(child => child.name === "colors") || { args: [] }).args
    readonly property var paints: layerValues ? layerValues.colors : []
    readonly property var fields: BorderStyles.fieldsOf(kind).concat(BorderStyles.commonFieldsOf(kind))
    readonly property bool keyboardInside: {
        let item = Window.activeFocusItem;
        if (!item || !item.visualFocus) {
            return false;
        }
        while (item) {
            if (item === card) {
                return true;
            }
            item = item.parent;
        }
        return false;
    }
    readonly property bool pointing: hover.hovered || keyboardInside

    function changeKind(next) {
        if (layerValues && layerValues.count > BorderStyles.maxCount(next)) {
            SettingsStore.setValue(layerPath + "/count", [BorderStyles.maxCount(next)]);
        }
        SettingsStore.setNode(layerPath, { name: "layer", args: [next], props: {} });
    }

    Layout.topMargin: Kirigami.Units.smallSpacing

    HoverHandler {
        id: hover
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.smallSpacing

        QQC2.ToolButton {
            icon.name: card.expanded ? "go-down-symbolic" : "go-next-symbolic"
            display: QQC2.AbstractButton.IconOnly
            text: card.expanded ? "Hide settings" : "Show settings"
            onClicked: card.toggled()
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
        }

        Item {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 4
            Layout.preferredHeight: Kirigami.Units.gridUnit * 2.5
            clip: true

            StyleBackdrop {
                anchors.fill: parent
            }

            MockWindow {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing * 2
                zoom: 1
                bare: true
                state_: "active"
                cornerRadius: 3
                ring: ({ enabled: false, width: card.baseWidth })
                overridePart: "focus-ring"
                overrideLayers: card.layerValues ? [card.layerValues] : []
            }
        }

        QQC2.ComboBox {
            Accessible.name: "Layer type"
            model: BorderStyles.kinds
            textRole: "label"
            valueRole: "value"
            currentIndex: Math.max(0, BorderStyles.kinds.findIndex(entry => entry.value === card.kind))
            onActivated: index => card.changeKind(BorderStyles.kinds[index].value)
            QQC2.ToolTip.text: card.kindInfo.tooltip
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        SignalBadge {
            layers: card.layerValues ? [card.layerValues] : []
        }

        Item {
            Layout.fillWidth: true
        }

        Repeater {
            model: [
                { icon: "arrow-up", text: "Move up, in front", action: "movedUp", allowed: card.index > 0 },
                { icon: "arrow-down", text: "Move down, behind", action: "movedDown", allowed: card.index < card.total - 1 },
                { icon: "edit-delete", text: "Remove this layer", action: "removed", allowed: true }
            ]

            QQC2.ToolButton {
                required property var modelData
                icon.name: modelData.icon
                display: QQC2.AbstractButton.IconOnly
                text: modelData.text
                enabled: modelData.allowed
                onClicked: card[modelData.action]()
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }
        }
    }

    Loader {
        active: card.expanded
        asynchronous: true
        visible: active
        Layout.fillWidth: true

        sourceComponent: ColumnLayout {
            spacing: 0

            QQC2.Label {
                visible: card.kind === "ripple"
                text: "Plays once each time the state changes or an event effect starts it. Replayed here so you can see it."
                wrapMode: Text.Wrap
                opacity: 0.75
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing * 2
                Layout.rightMargin: Kirigami.Units.largeSpacing * 2
                Layout.bottomMargin: Kirigami.Units.smallSpacing
            }

            BorderColorsRow {
                Layout.fillWidth: true
                description: card.kindInfo.colorsHint
                layerPath: card.layerPath
                rawColors: card.rawColors
                paints: card.paints
                layerValues: card.layerValues
            }

            FireOnRow {
                visible: BorderStyles.canFire(card.kind)
                Layout.fillWidth: true
                layerPath: card.layerPath
                layerValues: card.layerValues
            }

            SettingRow {
                visible: card.kind === "comet" || card.kind === "particles" || card.kindInfo.directed === true
                Layout.fillWidth: true
                label: "Direction follows"
                description: "Flip the way it runs with a signal, such as the scroll direction"

                DriveButton {
                    layerPath: card.layerPath
                    setting: "direction"
                    settingLabel: "Direction"
                    layerValues: card.layerValues
                }
            }

            BorderLayerChoices {
                Layout.fillWidth: true
                layerPath: card.layerPath
                layerValues: card.layerValues
                kind: card.kind
                baseWidth: card.baseWidth
            }

            Repeater {
                model: card.fields

                SliderRow {
                    required property var modelData
                    Layout.fillWidth: true
                    readonly property string path: card.layerPath + "/" + modelData.key
                    label: modelData.label
                    description: modelData.key === "speed" ? BorderStyles.speedDescription(card.kind) : modelData.description || ""
                    value: card.layerValues ? card.layerValues[modelData.key] : 0
                    from: modelData.from
                    to: modelData.to
                    stepSize: modelData.step
                    decimals: modelData.decimals
                    unit: modelData.unit || ""
                    resetPaths: [path]
                    onEdited: number => SettingsStore.setValue(path, [Number(number.toFixed(modelData.decimals))])

                    DriveButton {
                        visible: !modelData.undriven
                        layerPath: card.layerPath
                        setting: modelData.key
                        settingLabel: modelData.label
                        layerValues: card.layerValues
                        amountRange: Math.max(Math.abs(modelData.to), Math.abs(modelData.from), 1)
                    }
                }
            }
        }
    }
}
