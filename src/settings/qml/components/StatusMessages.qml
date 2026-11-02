import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.konveyor.settings

ColumnLayout {
    id: root

    spacing: 0

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Error
        visible: SettingsStore.configError.length > 0
        text: "The config has an error, so it can't be applied yet: " + SettingsStore.configError
    }

    Kirigami.InlineMessage {
        id: editError
        Layout.fillWidth: true
        position: Kirigami.InlineMessage.Position.Header
        property bool saveFailed: false
        property bool saveJustFailed: false
        type: saveFailed ? Kirigami.MessageType.Error : Kirigami.MessageType.Warning
        showCloseButton: true
        visible: false
        actions: Kirigami.Action {
            visible: editError.saveFailed
            icon.name: "document-edit"
            text: "Open config.kdl"
            onTriggered: SettingsStore.openConfigFile()
        }

        Connections {
            target: SettingsStore
            function onSaveFailed() {
                editError.saveJustFailed = true;
            }
            function onEditFailed(message) {
                editError.saveFailed = editError.saveJustFailed;
                editError.saveJustFailed = false;
                editError.text = editError.saveFailed ? message + "\nThe setting went back to its saved value. The same message is in the log." : message;
                editError.visible = true;
            }
            function onSaved() {
                if (editError.saveFailed) {
                    editError.visible = false;
                }
            }
        }
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Information
        visible: !SettingsStore.live.running
        text: "Konveyor isn't running right now. You can still change settings; they apply when it starts."
    }
}
