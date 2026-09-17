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
        type: Kirigami.MessageType.Warning
        showCloseButton: true
        visible: false

        Connections {
            target: SettingsStore
            function onEditFailed(message) {
                editError.text = message;
                editError.visible = true;
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
