import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import "lib"

Item {
    id: full

    Layout.minimumWidth: Kirigami.Units.gridUnit * 16
    Layout.minimumHeight: Kirigami.Units.gridUnit * 14
    Layout.preferredWidth: Kirigami.Units.gridUnit * 24
    Layout.preferredHeight: Kirigami.Units.gridUnit * 34

    readonly property var tabDefs: [
        { key: "all", label: i18n("All") },
        { key: "ingame", label: i18n("In Game") },
        { key: "online", label: i18n("Online") },
        { key: "favorites", label: i18n("Favourites") }
    ]

    Loader {
        id: loader
        anchors.fill: parent
        active: root.popupAlive
        sourceComponent: shellComponent
        onLoaded: if (root.expanded && root.inPanel) item.focusSearch()
    }

    Connections {
        target: root
        function onExpandedChanged() {
            if (root.expanded && loader.item)
                loader.item.focusSearch()
        }
    }

    Component {
        id: shellComponent

        PopupShell {
            id: shell

            anchors.fill: parent
            icon: root.panelIcon
            title: i18n("Steam Friends")
            subtitle: root.error !== "" ? i18n("Not connected")
                    : !root.ready ? i18n("Waiting for the collector…")
                    : i18n("%1 online  ·  %2 in game  ·  %3 friends", root.onlineCount, root.inGameCount, root.friends.length)
            statusColor: root.error !== "" ? Kirigami.Theme.negativeTextColor : root.onlineCount > 0 ? root.cInGame : Kirigami.Theme.neutralTextColor
            statusText: root.error !== "" ? root.error : i18n("Live presence from Steam")
            searchPlaceholder: i18n("Search friends or games…")
            tabs: full.tabDefs.map(tab => ({
                key: tab.key, label: tab.label,
                badge: tab.key === "ingame" ? root.inGameCount + "" : tab.key === "online" ? root.onlineCount + "" : tab.key === "favorites" && root.favoriteCount > 0 ? root.favoriteCount + "" : ""
            }))
            currentTab: Math.max(0, full.tabDefs.findIndex(tab => tab.key === root.tabKey))
            matchCount: root.searchText !== "" ? root.rows.count : -1
            onTabActivated: index => root.tabKey = full.tabDefs[index].key
            onCloseRequested: root.expanded = false
            onSearchTextChanged: root.searchText = searchText
            onSearchAccepted: {
                if (root.rows.count > 0)
                    root.openChat(root.friendsById[root.rows.get(0).steamid])
            }
            Component.onCompleted: if (root.searchText !== "") shell.searchText = root.searchText

            Connections {
                target: root
                function onMenuRequested(friend) {
                    friendMenu.friend = friend
                    friendMenu.popup()
                }
            }

            headerActions: [
                PlasmaComponents.ToolButton {
                    icon.name: root.sortMode === "name_desc" ? "view-sort-descending" : "view-sort-ascending"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Sort & filter")
                    onClicked: sortMenu.popup()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                    QQC2.Menu {
                        id: sortMenu
                        QQC2.MenuItem {
                            text: i18n("Name (A–Z)"); icon.name: "view-sort-ascending"
                            checkable: true; checked: root.sortMode === "name"
                            onTriggered: Plasmoid.configuration.sortMode = "name"
                        }
                        QQC2.MenuItem {
                            text: i18n("Name (Z–A)"); icon.name: "view-sort-descending"
                            checkable: true; checked: root.sortMode === "name_desc"
                            onTriggered: Plasmoid.configuration.sortMode = "name_desc"
                        }
                        QQC2.MenuSeparator {}
                        QQC2.MenuItem {
                            text: i18n("Hide offline"); icon.name: "im-invisible-user"
                            checkable: true; checked: root.hideOffline
                            onTriggered: Plasmoid.configuration.hideOffline = checked
                        }
                        QQC2.MenuItem {
                            text: i18n("Show who's playing"); icon.name: "applications-games"
                            checkable: true; checked: Plasmoid.configuration.showPlayingNow
                            onTriggered: Plasmoid.configuration.showPlayingNow = checked
                        }
                    }
                },
                PlasmaComponents.ToolButton {
                    icon.name: "view-refresh"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Refresh")
                    onClicked: root.read()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                },
                PlasmaComponents.ToolButton {
                    icon.name: "configure"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Configure…")
                    onClicked: Plasmoid.internalAction("configure").trigger()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                }
            ]

            ColumnLayout {
                anchors.fill: parent
                spacing: Kirigami.Units.largeSpacing

                PlayingNow {
                    Layout.fillWidth: true
                    visible: Plasmoid.configuration.showPlayingNow && root.error === "" && shell.searchText === ""
                             && (root.tabKey === "all" || root.tabKey === "ingame") && root.playingNow.length > 0
                    onGameClicked: game => shell.searchText = game
                }

                ListView {
                    id: listView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    reuseItems: true
                    cacheBuffer: Kirigami.Units.gridUnit * 20
                    model: root.rows
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds
                    QQC2.ScrollBar.vertical: QQC2.ScrollBar {}

                    section.property: "section"
                    section.criteria: ViewSection.FullString
                    section.delegate: RowLayout {
                        required property string section
                        width: ListView.view ? ListView.view.width : 0
                        height: implicitHeight + Kirigami.Units.smallSpacing * 2
                        spacing: Kirigami.Units.smallSpacing
                        PlasmaComponents.Label {
                            Layout.leftMargin: Kirigami.Units.smallSpacing
                            Layout.topMargin: Kirigami.Units.smallSpacing
                            text: section
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            font.weight: Font.DemiBold
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 0.6
                            opacity: 0.65
                        }
                        PlasmaComponents.Label {
                            Layout.topMargin: Kirigami.Units.smallSpacing
                            text: root.sectionCounts[section] || ""
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            font.features: { "tnum": 1 }
                            opacity: 0.45
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.topMargin: Kirigami.Units.smallSpacing
                            Layout.rightMargin: Kirigami.Units.smallSpacing
                            implicitHeight: 1
                            color: Qt.alpha(Kirigami.Theme.textColor, 0.1)
                        }
                    }

                    delegate: FriendRow {
                        width: ListView.view ? ListView.view.width - Kirigami.Units.smallSpacing : 0
                    }

                }
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: root.error === "" && root.ready && root.rows.count === 0
                iconName: root.searchText !== "" ? "edit-find" : "im-user"
                text: root.searchText !== "" ? i18n("No friends match")
                    : root.tabKey === "favorites" ? i18n("No favourites yet")
                    : root.tabKey === "ingame" ? i18n("Nobody is playing right now")
                    : root.tabKey === "online" || root.hideOffline ? i18n("No friends online")
                    : i18n("No friends to show")
                explanation: root.tabKey === "favorites" && root.searchText === "" ? i18n("Right-click a friend to pin them here.") : ""
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: !root.ready && root.error === ""
                iconName: "im-user"
                text: i18n("Waiting for Steam")
                explanation: i18n("The friends collector hasn't written a snapshot yet.")
            }

            SetupCard {
                anchors.fill: parent
                visible: root.error !== ""
            }

            QQC2.Menu {
                id: friendMenu
                property var friend: null
                QQC2.MenuItem {
                    text: i18n("Open Chat"); icon.name: "mail-message"
                    onTriggered: root.openChat(friendMenu.friend)
                }
                QQC2.MenuItem {
                    text: i18n("Join Game"); icon.name: "media-playback-start"
                    visible: !!(friendMenu.friend && friendMenu.friend.join)
                    height: visible ? implicitHeight : 0
                    onTriggered: root.steamRun(friendMenu.friend.join)
                }
                QQC2.MenuItem {
                    text: i18n("Watch Game"); icon.name: "video-television"
                    visible: !!(friendMenu.friend && friendMenu.friend.ingame)
                    height: visible ? implicitHeight : 0
                    onTriggered: root.steamRun(friendMenu.friend.watch)
                }
                QQC2.MenuSeparator {}
                QQC2.MenuItem {
                    text: i18n("View Profile"); icon.name: "steam"
                    onTriggered: root.steamRun(friendMenu.friend.profile)
                }
                QQC2.MenuItem {
                    text: i18n("Open Profile in Browser"); icon.name: "internet-web-browser"
                    visible: !!(friendMenu.friend && friendMenu.friend.profile_web)
                    height: visible ? implicitHeight : 0
                    onTriggered: Qt.openUrlExternally(friendMenu.friend.profile_web)
                }
                QQC2.MenuItem {
                    text: i18n("Copy Name"); icon.name: "edit-copy"
                    onTriggered: {
                        clipboard.text = friendMenu.friend.name || ""
                        clipboard.selectAll()
                        clipboard.copy()
                    }
                }
                QQC2.MenuSeparator {}
                QQC2.MenuItem {
                    text: friendMenu.friend && root.isFavorite(friendMenu.friend.steamid) ? i18n("Remove from Favourites") : i18n("Add to Favourites")
                    icon.name: "starred-symbolic"
                    onTriggered: root.toggleFavorite(friendMenu.friend.steamid)
                }
            }
            TextEdit { id: clipboard; visible: false }
        }
    }
}
