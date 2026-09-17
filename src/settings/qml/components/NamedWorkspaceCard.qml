import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../sections"
import org.kde.konveyor.settings

ColumnLayout {
    id: card

    required property var entry
    required property int index
    required property int count
    property bool expanded: false
    signal renameRequested(string path, string name)

    readonly property string path: entry.path
    readonly property string name: String((entry.node.args || [])[0] || "")
    readonly property var nodeChildren: entry.node.children || []
    readonly property var outputNode: nodeChildren.find(child => child.name === "open-on-output")
    readonly property string output: outputNode ? String(outputNode.args[0]) : ""
    readonly property bool hasLayout: nodeChildren.some(child => child.name === "layout")

    Layout.fillWidth: true
    spacing: Kirigami.Units.largeSpacing

    Card {
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            Rectangle {
                implicitWidth: Kirigami.Units.gridUnit * 2.2
                implicitHeight: implicitWidth
                radius: Kirigami.Units.cornerRadius * 2
                color: Qt.alpha(Kirigami.Theme.highlightColor, 0.18)

                QQC2.Label {
                    anchors.centerIn: parent
                    text: card.index + 1
                    font.bold: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                Kirigami.Heading {
                    level: 4
                    text: card.name
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                QQC2.Label {
                    text: (card.output.length ? "Opens on " + card.output : "Opens on any monitor") + (card.hasLayout ? " · custom layout" : "")
                    font: Kirigami.Theme.smallFont
                    opacity: 0.7
                    Layout.fillWidth: true
                }
            }

            QQC2.ToolButton {
                icon.name: "edit-rename"
                display: QQC2.AbstractButton.IconOnly
                text: "Rename"
                onClicked: card.renameRequested(card.path, card.name)
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }

            QQC2.ToolButton {
                icon.name: "arrow-up"
                display: QQC2.AbstractButton.IconOnly
                text: "Move up"
                enabled: card.index > 0
                onClicked: SettingsStore.move(card.path, -1)
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }

            QQC2.ToolButton {
                icon.name: "arrow-down"
                display: QQC2.AbstractButton.IconOnly
                text: "Move down"
                enabled: card.index < card.count - 1
                onClicked: SettingsStore.move(card.path, 1)
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }

            QQC2.ToolButton {
                icon.name: "edit-delete"
                display: QQC2.AbstractButton.IconOnly
                text: "Remove"
                onClicked: SettingsStore.remove(card.path)
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }
        }

        SettingRow {
            label: "Opens on monitor"
            description: "Where this workspace lives when Konveyor starts or the monitor connects."
            iconName: "video-display"
            resetPaths: [card.path + "/open-on-output"]

            OutputPicker {
                value: card.output
                allowNone: true
                onPicked: name => name.length ? SettingsStore.setValue(card.path + "/open-on-output", [name]) : SettingsStore.remove(card.path + "/open-on-output")
            }
        }

        SettingRow {
            label: "Layout for this workspace"
            description: card.hasLayout ? "Some layout settings are different here." : "Uses the same layout as everywhere else."
            iconName: "view-split-left-right"
            resetPaths: [card.path + "/layout"]

            QQC2.Button {
                text: card.expanded ? "Hide" : "Customize"
                icon.name: card.expanded ? "arrow-up" : "arrow-down"
                onClicked: card.expanded = !card.expanded
            }
        }
    }

    Loader {
        active: card.expanded
        visible: active
        Layout.fillWidth: true
        sourceComponent: LayoutOverrides {
            scopePath: card.path + "/layout"
            excluded: ["empty-workspace-above-first", "insert-hint", "group-app-windows", "max-rows-per-column", "new-window-placement"]
        }
    }
}
