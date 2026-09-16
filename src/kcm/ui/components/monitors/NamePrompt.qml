import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.PromptDialog {
    id: prompt

    property string label
    property var taken: []
    property string original
    readonly property string trimmed: field.text.trim()
    readonly property bool valid: trimmed.length > 0 && (trimmed === original || !taken.includes(trimmed))
    signal named(string name)

    function ask(current) {
        original = current;
        field.text = current;
        open();
    }

    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    onOpened: field.forceActiveFocus()
    onAccepted: {
        if (valid && trimmed !== original) {
            named(trimmed);
        }
    }

    QQC2.TextField {
        id: field
        placeholderText: prompt.label
        onAccepted: {
            if (prompt.valid) {
                prompt.accept();
            }
        }
    }
}
