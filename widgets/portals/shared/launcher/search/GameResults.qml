import QtQuick
import org.kde.kirigami as Kirigami
import ".."

ResultGroup {
    function grids() {
        return [grid]
    }
    title: i18n("Games")
    cellHeight: Kirigami.Units.gridUnit * 3.6
    model: page.gameRows
    delegate: RowTile {
        id: gameRow
        required property int index
        required property var modelData
        readonly property var grid: GridView.view
        readonly property var playing: launcherData.friendsFor(modelData)
        sidebarEntry: launcherData.sidebarEntryForGame(modelData)
        width: grid.cellWidth
        height: grid.cellHeight
        game: modelData.appid ? modelData : null
        iconSource: modelData.icon || "applications-games"
        label: modelData.name
        query: page.term
        subtitle: playing.length > 0 ? i18np("%1 friend playing", "%1 friends playing", playing.length) : modelData.last > 0 ? i18n("Played %1", launcherData.relativeTime(modelData.last)) : modelData.appid ? i18n("Steam game") : i18n("Game")
        subtitleColor: playing.length > 0 ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
        selected: GridView.isCurrentItem && grid.sectionActive
        function activate() {
            launcher.launchGame(modelData)
        }
        function openMenu() {
            launcher.openMenu(launcher.gameEntries(modelData), gameRow)
        }
        onHovered: launcher.select(grid, index)
        onClicked: activate()
        onRightClicked: {
            launcher.select(grid, index)
            openMenu()
        }
    }
}
