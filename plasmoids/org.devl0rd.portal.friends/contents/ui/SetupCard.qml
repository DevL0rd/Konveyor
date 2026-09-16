import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import "lib"

Rectangle {
    id: setup

    color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.94)
    radius: Kirigami.Units.cornerRadius * 2

    MouseArea { anchors.fill: parent }

    PopCard {
        anchors.centerIn: parent
        width: Math.min(parent.width, Kirigami.Units.gridUnit * 22)
        title: i18n("Connect Steam")
        icon: "dialog-password"
        trailing: i18n("Needs a key")
        trailingColor: Kirigami.Theme.neutralTextColor

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                source: "steam"
                Layout.preferredWidth: Kirigami.Units.iconSizes.large
                Layout.preferredHeight: Kirigami.Units.iconSizes.large
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Kirigami.Heading {
                    level: 4
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: i18n("Steam Web API key needed")
                }
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    opacity: 0.7
                    font: Kirigami.Theme.smallFont
                    text: i18n("The friends collector couldn't authenticate: %1", root.error)
                }
            }
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            textFormat: Text.StyledText
            text: i18n('Get a free key at <a href="https://steamcommunity.com/dev/apikey">steamcommunity.com/dev/apikey</a>')
            onLinkActivated: function(link) { Qt.openUrlExternally(link) }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing
            QQC2.TextField {
                id: keyField
                Layout.fillWidth: true
                placeholderText: i18n("Paste your API key")
                echoMode: TextInput.Password
                enabled: !root.saving
                onAccepted: root.saveKey(text)
            }
            PlasmaComponents.Button {
                text: root.saving ? i18n("Saving…") : i18n("Save")
                enabled: !root.saving && keyField.text !== ""
                icon.name: "dialog-ok-apply"
                onClicked: root.saveKey(keyField.text)
            }
        }
    }
}
