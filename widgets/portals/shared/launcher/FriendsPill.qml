import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.plasma.components as PlasmaComponents

MouseArea {
    id: pill

    readonly property var faces: {
        const playing = launcherData.friends.filter(friend => friend.ingame)
        const online = launcherData.friends.filter(friend => !friend.ingame && friend.state > 0)
        return playing.concat(online).slice(0, 3)
    }
    readonly property int inGame: launcherData.friendsInGame
    readonly property int online: Math.max(0, launcherData.friendsOnline - launcherData.friendsInGame)

    visible: launcherData.friendsEnabled && launcherData.friends.length > 0
    implicitWidth: row.implicitWidth + Kirigami.Units.largeSpacing * 2
    implicitHeight: Kirigami.Units.gridUnit * 2.1
    hoverEnabled: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    onPressAndHold: function(event) {
        if (launcher.touchMode)
            launcher.openMenu(launcher.friendsQuickEntries(pill), pill)
        else
            event.accepted = false
    }
    onClicked: function(event) {
        if (event.button === Qt.RightButton)
            launcher.openMenu(launcher.friendsQuickEntries(pill), pill)
        else
            launcher.goToPage("friends")
    }

    QQC2.ToolTip.visible: containsMouse && !menu.visible
    QQC2.ToolTip.text: i18n("Friends — click to open, right-click for quick actions")

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: pill.containsMouse ? launcher.selectedFill : launcher.well
        border.width: 1
        border.color: launcher.hairline
    }

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: Kirigami.Units.smallSpacing * 1.5

        Item {
            Layout.preferredWidth: pill.faces.length > 0 ? Kirigami.Units.iconSizes.smallMedium + (pill.faces.length - 1) * Kirigami.Units.iconSizes.smallMedium * 0.6 : 0
            Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
            Repeater {
                model: pill.faces
                Item {
                    required property var modelData
                    required property int index
                    x: index * Kirigami.Units.iconSizes.smallMedium * 0.6
                    z: 3 - index
                    width: Kirigami.Units.iconSizes.smallMedium
                    height: width
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -1.5
                        radius: width / 2
                        color: Kirigami.Theme.backgroundColor
                        border.width: 1.5
                        border.color: modelData.ingame ? Kirigami.Theme.positiveTextColor : Qt.alpha(Kirigami.Theme.textColor, 0.35)
                    }
                    Components.Avatar {
                        anchors.fill: parent
                        source: modelData.avatar || ""
                        name: modelData.name || ""
                    }
                }
            }
        }

        Rectangle {
            visible: pill.inGame > 0
            Layout.preferredWidth: Kirigami.Units.smallSpacing * 1.5
            Layout.preferredHeight: Layout.preferredWidth
            radius: width / 2
            color: Kirigami.Theme.positiveTextColor
        }

        PlasmaComponents.Label {
            text: launcher.compact ? (pill.inGame > 0 ? pill.inGame + "" : pill.online + "") : pill.inGame > 0 ? i18n("%1 in game · %2 online", pill.inGame, pill.online) : i18np("%1 online", "%1 online", pill.online)
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            font.weight: Font.DemiBold
            opacity: 0.85
        }
    }
}
