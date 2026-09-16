import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root

    spacing: 0

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Error
        visible: kcm.configError.length > 0
        text: "The config has an error, so it can't be applied yet: " + kcm.configError
    }

    Kirigami.InlineMessage {
        id: editError
        Layout.fillWidth: true
        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Warning
        showCloseButton: true
        visible: false

        Connections {
            target: kcm
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
        visible: !kcm.live.running
        text: "Konveyor isn't running right now. You can still change settings; they apply when it starts."
    }
}
