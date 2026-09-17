import QtQuick

Tile {
    id: tile

    required property int index
    required property var model
    readonly property var grid: GridView.view
    readonly property string favoriteId: model.favoriteId || ""
    property var sourceModel: grid ? grid.model : null
    property int sourceIndex: index

    width: grid ? grid.cellWidth : 0
    height: grid ? grid.cellHeight : 0
    iconSize: grid ? grid.iconSize : 48
    iconSource: model.decoration
    label: model.display || ""
    badge: model.isNewlyInstalled === true
    selected: GridView.isCurrentItem && !!grid && grid.sectionActive

    function activate() {
        launcher.trigger(sourceModel, sourceIndex)
    }
    function openMenu() {
        launcher.openMenu(launcher.kickerEntries(sourceModel, sourceIndex, model.hasActionList ? model.actionList : [], favoriteId), tile)
    }

    onHovered: launcher.select(grid, index)
    onClicked: activate()
    onRightClicked: {
        launcher.select(grid, index)
        openMenu()
    }
}
