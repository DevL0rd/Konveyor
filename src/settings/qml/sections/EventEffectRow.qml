import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import "../catalog/BorderStyles.js" as BorderStyles
import "../catalog/EventEffects.js" as EventEffects
import org.kde.konveyor.settings

ColumnLayout {
    id: row

    required property var info
    required property var scope
    readonly property string event: info.value
    readonly property var value: SettingsStore.revision >= 0 ? EventEffects.resolved(SettingsStore, scope.values, scope.scopePath, event) : ({})
    readonly property string style: value.style || "none"
    readonly property bool active: style !== "none"
    readonly property var styles: SettingsStore.values["border-styles"] || []
    readonly property var layers: BorderStyles.layersNamed(styles, style)
    readonly property bool missing: active && !styles.some(entry => entry.name === style)
    readonly property bool modified: SettingsStore.revision >= 0 && !scope.overrideMode && EventEffects.isModified(SettingsStore, scope.scopePath, event)
    readonly property bool darkTheme: Kirigami.ColorUtils.brightnessForColor(Kirigami.Theme.backgroundColor) === Kirigami.ColorUtils.Dark

    signal chooseRequested()
    signal tried(string reach, bool on)
    signal replayed()

    function update(changes) {
        EventEffects.write(SettingsStore, scope.scopePath, event, Object.assign({}, value, changes));
    }

    spacing: 0

    Component {
        id: reachScene
        EventEffectScene {
            layers: row.layers
            reach: parent ? parent.choice.value : "self"
            duration: row.value.duration || 0
        }
    }

    ScopedRow {
        Layout.fillWidth: true
        scope: row.scope
        key: "event-effects"
        showOverride: false
        resetPaths: []
        label: row.info.label
        description: row.info.description
        iconName: row.info.icon
        wideControl: true

        RowLayout {
            width: parent.width
            spacing: Kirigami.Units.smallSpacing

            QQC2.AbstractButton {
                id: tile
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 22
                implicitHeight: Kirigami.Units.gridUnit * 4.5
                text: row.info.label + ": " + (row.active ? row.style : "None")
                hoverEnabled: true
                onClicked: row.chooseRequested()
                onHoveredChanged: row.tried(row.value.reach, hovered)
                onVisualFocusChanged: row.tried(row.value.reach, visualFocus)
                QQC2.ToolTip.text: "Choose the style this event plays"
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

                background: Rectangle {
                    radius: 5
                    color: Kirigami.Theme.backgroundColor
                    border.width: 1
                    border.color: tile.hovered || tile.visualFocus ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.3)
                }

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing * 2

                    EventEffectScene {
                        Layout.margins: 1
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 9
                        Layout.fillHeight: true
                        layers: row.layers
                        reach: row.value.reach || "self"
                        duration: row.value.duration || 0
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: row.active ? row.style : "None"
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: row.missing ? "Style not found" : row.active ? BorderStyles.summary(row.layers) : "Click to choose a style"
                            elide: Text.ElideRight
                            font: Kirigami.Theme.smallFont
                            opacity: row.missing ? 1 : 0.75
                            color: row.missing ? (row.darkTheme ? "#ff7b86" : "#b01f2e") : Kirigami.Theme.textColor
                        }
                    }

                    Kirigami.Icon {
                        Layout.rightMargin: Kirigami.Units.smallSpacing * 2
                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: implicitWidth
                        source: "arrow-right"
                    }
                }
            }

            QQC2.ToolButton {
                visible: row.active
                icon.name: "dialog-close-symbolic"
                display: QQC2.AbstractButton.IconOnly
                text: "Play nothing"
                onClicked: EventEffects.clear(SettingsStore, row.scope.scopePath, row.event, row.scope.overrideMode, row.value)
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }

            ModifiedBadge {
                visible: row.modified
                Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5
                Layout.preferredHeight: Kirigami.Units.gridUnit * 1.5
                onResetRequested: EventEffects.reset(SettingsStore, row.scope.scopePath, row.event)
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    SettingRow {
        visible: row.active
        Layout.fillWidth: true
        label: "Reach"
        description: "Which windows' borders play it. They light up even with no ring of their own."
        wideControl: true

        ChoiceCards {
            width: parent.width
            cardHeight: Kirigami.Units.gridUnit * 8.5
            currentValue: row.value.reach
            options: EventEffects.reaches.map(entry => ({ value: entry.value, title: entry.label, description: entry.caption, preview: reachScene }))
            onChosen: value => {
                row.update({ reach: value });
                row.replayed();
            }
            onTried: (value, on) => row.tried(value, on)
        }
    }

    SettingRow {
        visible: row.active && row.info.mode
        Layout.fillWidth: true
        label: "Plays"
        description: "Once on each event, or for as long as it keeps going, then fades"

        Segmented {
            options: EventEffects.modes
            currentValue: row.value.mode
            onChosen: value => row.update({ mode: value })
        }
    }

    SliderRow {
        visible: row.active
        Layout.fillWidth: true
        label: "Duration"
        description: "0 uses the style's own length"
        value: row.value.duration || 0
        from: 0
        to: 10
        stepSize: 0.1
        decimals: 1
        unit: "s"
        onEdited: number => row.update({ duration: Number(number.toFixed(1)) })
    }
}
