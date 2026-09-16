import QtQuick

Tile {
    id: tile

    required property int index
    required property var model
    readonly property var grid: GridView.view
    readonly property string favoriteId: model.favoriteId || ""

    width: grid.cellWidth
    height: grid.cellHeight
    iconSize: grid.iconSize
    iconSource: model.decoration
    label: model.display || ""
    badge: model.isNewlyInstalled === true
    selected: GridView.isCurrentItem && grid.sectionActive

    function activate() {
        launcher.trigger(grid.model, index)
    }
    function openMenu() {
        launcher.openMenu(launcher.kickerEntries(grid.model, index, model.hasActionList ? model.actionList : [], favoriteId), tile)
    }

    onHovered: launcher.select(grid, index)
    onClicked: activate()
    onRightClicked: {
        launcher.select(grid, index)
        openMenu()
    }
}
