import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import ".."

PopScroll {
    id: page

    readonly property var sections: [inGame, online, offline]
    readonly property var inGameList: launcherData.friends.filter(friend => friend.ingame)
    readonly property var onlineList: launcherData.friends.filter(friend => !friend.ingame && friend.state > 0)
    readonly property var offlineList: launcherData.friends.filter(friend => !friend.ingame && !(friend.state > 0))
    readonly property int columnWidth: Kirigami.Units.gridUnit * 16

    PlasmaComponents.Label {
        visible: launcherData.friends.length === 0
        Layout.fillWidth: true
        text: i18n("No friends loaded. Add your Steam Web API key in the Steam Friends widget settings.")
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
    }
    TileGrid {
        id: offline
        visible: count > 0
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
