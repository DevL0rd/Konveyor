import QtQuick

Tile {
    id: tile

    required property int index
    required property var model
    readonly property var grid: GridView.view
    readonly property string favoriteId: model.favoriteId || ""
    property var sourceModel: grid ? grid.model : null
    property int sourceIndex: index
    sidebarEntry: launcherData.sidebarEntryFor(favoriteId, model.url, model.display)

    width: grid ? grid.cellWidth : 0
    height: grid ? grid.cellHeight : 0
    iconSize: grid ? grid.iconSize : 48
    iconSource: model.decoration
    label: model.display || ""
    badge: model.isNewlyInstalled === true
    game: launcherData.gameForApp(favoriteId)
    selected: GridView.isCurrentItem && !!grid && grid.sectionActive
    reorderable: !!grid && grid.reorderable === true
    dropTarget: !!grid && grid.dropIndex === index && grid.dragIndex !== index && grid.dragIndex >= 0

    function gridIndexAt(position) {
        const mapped = tile.mapToItem(grid.contentItem, position.x, position.y)
        return grid.indexAt(mapped.x, mapped.y)
    }
    onReorderMove: function(position) {
        grid.dragIndex = index
        grid.dropIndex = gridIndexAt(position)
    }
    onReorderCancel: {
        grid.dragIndex = -1
        grid.dropIndex = -1
    }
    onReorderDrop: function(position) {
        const target = gridIndexAt(position)
        grid.dragIndex = -1
        grid.dropIndex = -1
        if (target >= 0 && target !== index && sourceModel && sourceModel.moveRow)
            sourceModel.moveRow(index, target)
    }

    function activate() {
        launcher.trigger(sourceModel, sourceIndex)
    }
    function openMenu() {
        launcher.openMenu(launcher.kickerEntries(sourceModel, sourceIndex, model.hasActionList ? model.actionList : [], favoriteId, model.url), tile)
    }

    onHovered: launcher.select(grid, index)
    onClicked: activate()
    onRightClicked: {
        launcher.select(grid, index)
        openMenu()
    }
}
