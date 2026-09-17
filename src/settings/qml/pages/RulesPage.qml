import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../catalog/Kdl.js" as Kdl
import "../catalog/RuleSummary.js" as RuleSummary
import org.kde.konveyor.settings

SettingsPage {
    id: page

    title: "Window Rules"
    readonly property var rules: SettingsStore.revision >= 0 ? SettingsStore.children("", "window-rule") : []

    function openRule(path) {
        SettingsNavigation.push("pages/RuleEditorPage.qml", { rulePath: path });
    }

    Component.onCompleted: SettingsStore.live.refresh()

    CardHeader {
        title: "Rules"
        trailing: QQC2.Button {
            icon.name: "list-add"
            text: "Add rule"
            onClicked: {
                const path = SettingsStore.append("", Kdl.block("window-rule", [Kdl.leaf("match", [], {})]));
                if (path.length) {
                    page.openRule(path);
                }
            }
        }
    }

    Card {
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: true
            type: Kirigami.MessageType.Information
            text: "Rules apply from top to bottom. When two rules change the same thing, the one further down wins."
        }

        Kirigami.PlaceholderMessage {
            visible: page.rules.length === 0
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.gridUnit
            icon.name: "preferences-system-windows-actions"
            text: "No window rules yet"
            explanation: "Rules let specific apps open floating, at a certain width, pinned to one end of the row, and more."
        }

        Repeater {
            model: page.rules

            RuleCard {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                rule: modelData.node
                isFirst: index === 0
                isLast: index === page.rules.length - 1
                onOpened: page.openRule(modelData.path)
                onMovedUp: SettingsStore.move(modelData.path, -1)
                onMovedDown: SettingsStore.move(modelData.path, 1)
                onDeleted: SettingsStore.remove(modelData.path)
            }
        }
    }

    component RuleCard: QQC2.ItemDelegate {
        id: card

        property var rule
        property bool isFirst
        property bool isLast
        signal opened
        signal movedUp
        signal movedDown
        signal deleted

        readonly property var apps: RuleSummary.appIds(rule)

        onClicked: opened()
        padding: Kirigami.Units.largeSpacing

        contentItem: RowLayout {
            spacing: Kirigami.Units.largeSpacing

            Item {
                Layout.preferredWidth: Kirigami.Units.iconSizes.large
                Layout.preferredHeight: Kirigami.Units.iconSizes.large

                Kirigami.Icon {
                    anchors.fill: parent
                    visible: card.apps.length === 0
                    source: RuleSummary.child(card.rule, "manage") ? "window-duplicate" : "window"
                    opacity: 0.8
                }

                Repeater {
                    model: card.apps.slice(0, 3)

                    Kirigami.Icon {
                        required property string modelData
                        required property int index
                        width: parent.width * (card.apps.length > 1 ? 0.7 : 1)
                        height: width
                        x: index * parent.width * 0.15
                        y: index * parent.height * 0.15
                        source: SettingsStore.live.iconFor(modelData)
                        fallback: "application-x-executable"
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing / 2

                QQC2.Label {
                    text: RuleSummary.target(card.rule, appId => SettingsStore.live.nameFor(appId))
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                QQC2.Label {
                    text: RuleSummary.summary(card.rule)
                    wrapMode: Text.Wrap
                    opacity: 0.75
                    Layout.fillWidth: true
                }
            }

            QQC2.ToolButton {
                icon.name: "go-up"
                text: "Move up"
                display: QQC2.AbstractButton.IconOnly
                enabled: !card.isFirst
                onClicked: card.movedUp()
                QQC2.ToolTip.text: "Move up (applies earlier)"
                QQC2.ToolTip.visible: hovered
            }

            QQC2.ToolButton {
                icon.name: "go-down"
                text: "Move down"
                display: QQC2.AbstractButton.IconOnly
                enabled: !card.isLast
                onClicked: card.movedDown()
                QQC2.ToolTip.text: "Move down (applies later, wins over earlier rules)"
                QQC2.ToolTip.visible: hovered
            }

            QQC2.ToolButton {
                icon.name: "edit-delete"
                text: "Delete rule"
                display: QQC2.AbstractButton.IconOnly
                onClicked: card.deleted()
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }

            Kirigami.Icon {
                source: "go-next"
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: Kirigami.Units.iconSizes.small
            }
        }
    }
}
