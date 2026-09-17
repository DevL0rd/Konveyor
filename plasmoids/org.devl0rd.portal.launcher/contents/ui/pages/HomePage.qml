import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import ".."

PopScroll {
    id: page

    readonly property var sections: [pinned, recentApps, playing, recentFiles]
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
            level: 1
            text: page.greeting
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        PlasmaComponents.Label {
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
        text: i18n("Right-click any app and choose Pin to Home, or press Ctrl+P on it.")
        opacity: 0.6
        wrapMode: Text.Wrap
    }
    TileGrid {
        id: pinned
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.round(page.tileSize + Kirigami.Units.gridUnit * 3.6)
        cellHeight: Math.round(page.tileSize + Kirigami.Units.gridUnit * 3.2)
        iconSize: page.tileSize
        model: launcherData.favorites
        delegate: KickerTile {}
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
        visible: playing.visible
        title: i18n("Continue playing")
        trailing: launcherData.friendsInGame > 0 ? i18np("%1 friend in game", "%1 friends in game", launcherData.friendsInGame) : ""
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
