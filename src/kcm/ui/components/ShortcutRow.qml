import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import "../catalog/Actions.js" as Actions

FormCard.AbstractFormDelegate {
    id: row

    property var bind
    property string modKey
    property bool conflicting
    readonly property var actionNode: bind && bind.children && bind.children.length ? bind.children[0] : null
    readonly property var actionInfo: actionNode ? Actions.byId(actionNode.name) : null
    readonly property var props: bind ? bind.props || {} : {}
    readonly property bool hidden: props.hasOwnProperty("hotkey-overlay-title") && props["hotkey-overlay-title"] === null
    readonly property string customTitle: typeof props["hotkey-overlay-title"] === "string" ? props["hotkey-overlay-title"] : ""
    readonly property string path: "binds/" + (bind ? bind.name : "")
    readonly property bool modified: kcm.revision >= 0 && !kcm.isDefault(path)

    signal editRequested
    signal removeRequested

    onClicked: editRequested()
    Accessible.name: (customTitle || Actions.describe(actionNode)) + ", " + (bind ? bind.name : "")

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            source: row.actionInfo ? row.actionInfo.icon : "dialog-question"
            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
            Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                Layout.fillWidth: true

                QQC2.Label {
                    text: row.customTitle || Actions.describe(row.actionNode)
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                ModifiedBadge {
                    visible: row.modified
                    onResetRequested: kcm.resetToDefault(row.path)
                }
            }

            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                visible: subtitle.text.length > 0 || row.conflicting

                QQC2.Label {
                    id: subtitle
                    text: [row.customTitle ? Actions.describe(row.actionNode) : "", row.hidden ? "Hidden from the cheatsheet" : "",
                        row.props.repeat === false ? "No repeat" : "", row.props["cooldown-ms"] ? "Cooldown " + row.props["cooldown-ms"] + " ms" : ""]
                        .filter(Boolean).join(" · ")
                    visible: text.length > 0
                    opacity: 0.7
                    font: Kirigami.Theme.smallFont
                    elide: Text.ElideRight
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 22
                }

                Rectangle {
                    visible: row.conflicting
                    implicitWidth: conflictLabel.implicitWidth + Kirigami.Units.largeSpacing
                    implicitHeight: conflictLabel.implicitHeight + 2
                    radius: height / 2
                    color: Qt.alpha(Kirigami.Theme.negativeTextColor, 0.15)
                    border.color: Kirigami.Theme.negativeTextColor

                    QQC2.Label {
                        id: conflictLabel
                        anchors.centerIn: parent
                        text: "Same trigger as another shortcut"
                        color: Kirigami.Theme.negativeTextColor
                        font: Kirigami.Theme.smallFont
                    }
                }
            }
        }

        KeyCaps {
            keyName: row.bind ? row.bind.name : ""
            modKey: row.modKey
        }

        QQC2.ToolButton {
            icon.name: "document-edit"
            display: QQC2.AbstractButton.IconOnly
            text: "Edit shortcut"
            onClicked: row.editRequested()
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        QQC2.ToolButton {
            icon.name: "edit-delete"
            display: QQC2.AbstractButton.IconOnly
            text: "Remove shortcut"
            onClicked: row.removeRequested()
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
    }
}
