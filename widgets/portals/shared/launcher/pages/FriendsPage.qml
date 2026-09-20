import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import ".."

PopScroll {
    id: page

    readonly property var sections: [playingCards, inGame, online, offline]
    property bool showAllOffline: false
    readonly property var inGameList: launcherData.friends.filter(friend => friend.ingame)
    readonly property var onlineList: launcherData.friends.filter(friend => !friend.ingame && friend.state > 0)
    readonly property var offlineList: launcherData.friends.filter(friend => !friend.ingame && !(friend.state > 0))
    readonly property int columnWidth: Kirigami.Units.gridUnit * 16
    readonly property bool needsApiKey: launcherData.friendsNeedsApiKey

    function saveApiKey() {
        if (!launcherData.steamKeyBusy && apiKey.text.trim() !== "")
            launcherData.setSteamApiKey(apiKey.text)
    }

    Connections {
        target: launcherData
        function onSteamKeyResultChanged() {
            if (launcherData.steamKeyResult !== "" && !launcherData.steamKeyError)
                apiKey.text = ""
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing
        Kirigami.Heading {
            level: 2
            text: i18n("Friends")
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            text: i18n("%1 in game · %2 online · %3 total", launcherData.friendsInGame, Math.max(0, launcherData.friendsOnline - launcherData.friendsInGame), launcherData.friends.length)
            opacity: 0.55
        }
        Segment {
            text: i18n("Open Steam friends")
            iconName: "system-users-symbolic"
            onClicked: launcher.openUrl("steam://open/friends")
        }
    }

    SectionHeader {
        visible: playingCards.visible
        title: i18n("Playing now")
        trailing: launcherData.playingNow.length + ""
    }
    TileGrid {
        id: playingCards
        visible: launcherData.playingNow.length > 0
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / (Kirigami.Units.gridUnit * 15))))
        cellHeight: Math.round(cellWidth * 0.4667)
        model: launcherData.playingNow
        delegate: PlayingNowCard {}
    }

    Rectangle {
        visible: page.needsApiKey
        Layout.fillWidth: true
        implicitHeight: apiSetup.implicitHeight + Kirigami.Units.largeSpacing * 2
        radius: Kirigami.Units.cornerRadius * 2
        color: launcher.well
        border.width: 1
        border.color: launcher.hairline

        ColumnLayout {
            id: apiSetup
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                level: 3
                text: i18n("Connect Steam Friends")
            }
            PlasmaComponents.Label {
                Layout.fillWidth: true
                text: i18n("Paste a Steam Web API key to load your friends.")
                wrapMode: Text.Wrap
                opacity: 0.7
            }
            RowLayout {
                Layout.fillWidth: true
                QQC2.TextField {
                    id: apiKey
                    Layout.fillWidth: true
                    placeholderText: i18n("Steam Web API key")
                    echoMode: TextInput.Password
                    onAccepted: page.saveApiKey()
                }
                QQC2.Button {
                    text: i18n("Get a key")
                    onClicked: launcher.openUrl("https://steamcommunity.com/dev/apikey")
                }
                QQC2.Button {
                    id: saveKey
                    text: launcherData.steamKeyBusy ? i18n("Saving…") : i18n("Save and connect")
                    enabled: !launcherData.steamKeyBusy && apiKey.text.trim() !== ""
                    onClicked: page.saveApiKey()
                }
            }
            PlasmaComponents.Label {
                visible: launcherData.steamKeyResult !== ""
                Layout.fillWidth: true
                text: launcherData.steamKeyResult
                wrapMode: Text.Wrap
                color: launcherData.steamKeyError ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.positiveTextColor
            }
        }
    }

    PlasmaComponents.Label {
        visible: launcherData.friends.length === 0 && !page.needsApiKey
        Layout.fillWidth: true
        text: launcherData.friendsError !== "" ? launcherData.friendsError : i18n("No friends loaded")
        wrapMode: Text.Wrap
        opacity: 0.6
    }

    SectionHeader {
        visible: inGame.count > 0
        title: i18n("In game")
        trailing: inGame.count + ""
    }
    TileGrid {
        id: inGame
        visible: count > 0
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / page.columnWidth)))
        cellHeight: Kirigami.Units.gridUnit * 3.2
        model: page.inGameList
        delegate: FriendRow {}
    }

    SectionHeader {
        visible: online.count > 0
        title: i18n("Online")
        trailing: online.count + ""
    }
    TileGrid {
        id: online
        visible: count > 0
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / page.columnWidth)))
        cellHeight: Kirigami.Units.gridUnit * 3.2
        model: page.onlineList
        delegate: FriendRow {}
    }

    SectionHeader {
        visible: offline.count > 0
        title: i18n("Offline")
        trailing: offline.count + ""
        actionText: offline.count > 10 ? (page.showAllOffline ? i18n("Show fewer") : i18n("Show all")) : ""
        onActionClicked: page.showAllOffline = !page.showAllOffline
    }
    TileGrid {
        id: offline
        visible: count > 0
        limit: page.showAllOffline ? -1 : 10
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / page.columnWidth)))
        cellHeight: Kirigami.Units.gridUnit * 3.2
        model: page.offlineList
        delegate: FriendRow {}
    }

    Item {
        Layout.fillHeight: true
    }
}
