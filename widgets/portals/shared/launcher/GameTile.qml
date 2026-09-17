import QtQuick
import org.kde.kirigami as Kirigami
import "lib"

Item {
    id: tile

    required property int index
    required property var modelData
    readonly property var grid: GridView.view
    readonly property var game: modelData
    readonly property var playing: launcherData.friendsFor(game)
    readonly property var sidebarEntry: launcherData.sidebarEntryForGame(game)

    width: grid.cellWidth
    height: grid.cellHeight

    function activate() {
        launcherData.launchGame(game)
        root.hide()
    }
    function openMenu() {
        launcher.openMenu(launcher.gameEntries(game), tile)
    }

    DragHandler {
        target: null
        yAxis.enabled: false
        grabPermissions: PointerHandler.CanTakeOverFromAnything
        onCentroidChanged: if (active) launcher.sidebarDragMove(tile, centroid.position.x, centroid.position.y, tile.sidebarEntry, -1, tile.game.icon)
        onActiveChanged: if (!active) launcher.sidebarDragEnd()
    }

    GameCard {
        anchors.fill: parent
        anchors.margins: tile.grid.cardSpacing !== undefined ? tile.grid.cardSpacing : Kirigami.Units.largeSpacing * 0.75
        game: tile.game
        wide: tile.grid.wideCards === true
        showTitle: tile.grid.showTitles !== false || launcher.touchMode
        friendCount: tile.playing.length
        friends: tile.playing
        selected: tile.GridView.isCurrentItem && tile.grid.sectionActive
        onHoveredChanged: if (hovered) launcher.select(tile.grid, tile.index)
        onCardClicked: tile.activate()
        onMenuRequested: {
            launcher.select(tile.grid, tile.index)
            tile.openMenu()
        }
    }
}
