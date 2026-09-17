import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import QtQuick.Controls as QQC2
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import ".."

PopScroll {
    id: page

    readonly property var sections: [pinned, folderGrid, recentApps, friendsPlaying, playing, recentFiles]
    readonly property int tileSize: Plasmoid.configuration.tileSize
    readonly property string greeting: {
        launcher.shown
        const hour = new Date().getHours()
        const name = launcherData.user.fullName || launcherData.user.loginName
        if (hour < 5)
            return i18n("Up late, %1", name)
        if (hour < 12)
            return i18n("Good morning, %1", name)
        if (hour < 18)
            return i18n("Good afternoon, %1", name)
        return i18n("Good evening, %1", name)
    }
    readonly property string today: {
        launcher.shown
        return Qt.formatDate(new Date(), Qt.locale().dateFormat(Locale.LongFormat))
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Kirigami.Units.smallSpacing
        Kirigami.Heading {
            level: launcher.compact ? 2 : 1
            text: page.greeting
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        PlasmaComponents.Label {
            visible: !launcher.compact
            text: page.today
            opacity: 0.6
        }
    }

    SectionHeader {
        title: i18n("Pinned")
        trailing: launcherData.favorites.count > 0 ? launcherData.favorites.count + "" : ""
    }
    PlasmaComponents.Label {
        visible: launcherData.favorites.count === 0
        Layout.fillWidth: true
        text: i18n("Pin apps here: right-click any app and choose Pin to Home, press Ctrl+P on it, or drop a .desktop file on the launcher button. Drag one pin onto another to make a folder.")
        opacity: 0.6
        wrapMode: Text.Wrap
    }
    TileGrid {
        id: pinned
        reorderable: true
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.round(page.tileSize + Kirigami.Units.gridUnit * 3.6)
        cellHeight: Math.round(page.tileSize + Kirigami.Units.gridUnit * 3.2)
        iconSize: page.tileSize
        model: launcherData.pinnedEntries
        delegate: PinnedTile {}
    }

    Rectangle {
        id: folderPanel
        readonly property var folder: launcherData.pinnedEntries.find(entry => entry.kind === "folder" && entry.id === launcher.openFolder) || null
        visible: folder !== null
        opacity: visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration } }
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? folderColumn.implicitHeight + Kirigami.Units.largeSpacing * 2 : 0
        radius: Kirigami.Units.cornerRadius * 3
        color: launcher.well
        border.width: 1
        border.color: launcher.hairline

        Connections {
            target: launcher
            function onRenameRequested(id) {
                Qt.callLater(() => {
                    folderName.forceActiveFocus()
                    folderName.selectAll()
                })
            }
        }

        ColumnLayout {
            id: folderColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.small
                    Layout.preferredHeight: Layout.preferredWidth
                    source: "folder-symbolic"
                    color: launcher.ink
                    isMask: true
                    opacity: 0.7
                }
                QQC2.TextField {
                    id: folderName
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 14
                    text: folderPanel.folder ? folderPanel.folder.name : ""
                    font.weight: Font.DemiBold
                    placeholderText: i18n("Folder name")
                    background: Rectangle {
                        radius: Kirigami.Units.cornerRadius
                        color: folderName.activeFocus ? Qt.alpha(launcher.ink, 0.08) : "transparent"
                        border.width: folderName.activeFocus ? 1 : 0
                        border.color: launcher.hairline
                    }
                    function commit() {
                        if (folderPanel.folder && text.trim() !== "" && text !== folderPanel.folder.name)
                            launcherData.renameFolder(folderPanel.folder.id, text)
                    }
                    onEditingFinished: commit()
                    Keys.onReturnPressed: function(event) {
                        event.accepted = true
                        commit()
                        Qt.callLater(launcher.focusSearch)
                    }
                    Keys.onEnterPressed: function(event) {
                        event.accepted = true
                        commit()
                        Qt.callLater(launcher.focusSearch)
                    }
                    Keys.onEscapePressed: {
                        text = folderPanel.folder ? folderPanel.folder.name : ""
                        launcher.focusSearch()
                    }
                }
                PlasmaComponents.Label {
                    text: folderPanel.folder ? i18np("%1 app", "%1 apps", folderPanel.folder.apps.length) : ""
                    opacity: 0.5
                }
                Item { Layout.fillWidth: true }
                PlasmaComponents.Label {
                    visible: !launcher.compact
                    text: i18n("Drag an app out to take it out of the folder")
                    font: Kirigami.Theme.smallFont
                    opacity: 0.45
                }
                PlasmaComponents.ToolButton {
                    icon.name: "edit-delete-remove"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Ungroup")
                    onClicked: {
                        const id = folderPanel.folder.id
                        launcher.openFolder = ""
                        launcherData.deleteFolder(id)
                    }
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                }
                PlasmaComponents.ToolButton {
                    icon.name: "window-close-symbolic"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Close folder")
                    onClicked: launcher.openFolder = ""
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                }
            }

            TileGrid {
                id: folderGrid
                visible: folderPanel.visible
                folderId: launcher.openFolder
                reorderable: true
                Layout.fillWidth: true
                Layout.preferredHeight: implicitHeight
                cellWidth: pinned.cellWidth
                cellHeight: pinned.cellHeight
                iconSize: page.tileSize
                model: folderPanel.folder ? folderPanel.folder.apps.map(index => ({ kind: "app", favIndex: index, favoriteId: launcherData.favoriteIds[index] })) : []
                delegate: PinnedTile {}
            }
        }
    }

    SectionHeader {
        visible: recentApps.visible
        title: i18n("Recently used")
    }
    TileGrid {
        id: recentApps
        visible: Plasmoid.configuration.showRecentApps && count > 0
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.round(page.tileSize + Kirigami.Units.gridUnit * 3.6)
        cellHeight: Math.round(page.tileSize + Kirigami.Units.gridUnit * 3.2)
        iconSize: page.tileSize
        limit: columns
        model: launcherData.recentApps
        delegate: KickerTile {}
    }

    SectionHeader {
        visible: friendsPlaying.visible
        title: i18n("Friends playing now")
        trailing: i18np("%1 friend in game", "%1 friends in game", launcherData.friendsInGame)
        actionText: i18n("All friends")
        onActionClicked: launcher.goToPage("friends")
    }
    TileGrid {
        id: friendsPlaying
        visible: Plasmoid.configuration.showFriends && launcherData.playingNow.length > 0
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / (Kirigami.Units.gridUnit * 15))))
        cellHeight: Math.round(cellWidth * 0.4667)
        limit: columns
        model: launcherData.playingNow
        delegate: PlayingNowCard {}
    }

    SectionHeader {
        visible: playing.visible
        title: i18n("Continue playing")
        actionText: i18n("All games")
        onActionClicked: launcher.goToPage("games")
    }
    TileGrid {
        id: playing
        visible: Plasmoid.configuration.showGames && launcherData.recentGames.length > 0
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / (Kirigami.Units.gridUnit * 15))))
        cellHeight: Math.round(cellWidth * 0.4667)
        wideCards: true
        showTitles: false
        limit: columns
        model: launcherData.recentGames
        delegate: GameTile {}
    }

    SectionHeader {
        visible: recentFiles.visible
        title: i18n("Recent files")
        actionText: i18n("All files")
        onActionClicked: launcher.goToPage("files")
    }
    TileGrid {
        id: recentFiles
        visible: Plasmoid.configuration.showRecentFiles && count > 0
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / (Kirigami.Units.gridUnit * 16))))
        cellHeight: Kirigami.Units.gridUnit * 2.8
        limit: columns * 2
        model: launcherData.recentDocs
        delegate: KickerRow {}
    }

    Item {
        Layout.fillHeight: true
    }
}
