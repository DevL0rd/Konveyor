import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.PromptDialog {
    id: dialog

    property var takenNames: []
    property string originalName
    signal nameChosen(string name)

    readonly property string trimmed: field.text.trim()
    readonly property bool duplicate: takenNames.some(name => name.toLowerCase() === trimmed.toLowerCase() && name.toLowerCase() !== originalName.toLowerCase())
    readonly property bool valid: trimmed.length > 0 && !duplicate

    function openWith(name) {
        originalName = name;
        field.text = name;
        open();
        field.forceActiveFocus();
        field.selectAll();
    }

    title: originalName.length ? "Rename workspace" : "New named workspace"
    standardButtons: Kirigami.Dialog.NoButton
    customFooterActions: [
        Kirigami.Action {
            text: dialog.originalName.length ? "Rename" : "Add"
            icon.name: "dialog-ok"
            enabled: dialog.valid
            onTriggered: {
                dialog.nameChosen(dialog.trimmed);
                dialog.close();
            }
        },
        Kirigami.Action {
            text: "Cancel"
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            text: "Named workspaces always exist, even when empty, and shortcuts can jump to them by name."
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            Layout.preferredWidth: Kirigami.Units.gridUnit * 20
        }

        QQC2.TextField {
            id: field
            placeholderText: "For example: Chat, Code, Games"
            Layout.fillWidth: true
            onAccepted: {
                if (dialog.valid) {
                    dialog.nameChosen(dialog.trimmed);
                    dialog.close();
                }
            }
        }

        QQC2.Label {
            visible: dialog.duplicate
            text: "There's already a workspace with that name."
            color: Kirigami.Theme.negativeTextColor
            font: Kirigami.Theme.smallFont
        }
    }
}
