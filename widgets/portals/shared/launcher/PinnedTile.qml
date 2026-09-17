import QtQuick

Tile {
    id: tile

    required property int index
    required property var modelData
    readonly property var grid: GridView.view
    readonly property bool folderEntry: modelData.kind === "folder"
    readonly property var row: {
        launcherData.favoriteIds
        return folderEntry ? null : launcherData.favoriteRow(modelData.favIndex)
    }
    readonly property string favoriteId: row ? row.favoriteId : ""
    readonly property bool insideFolder: !!grid && grid.folderId !== undefined && grid.folderId !== ""

    width: grid ? grid.cellWidth : 0
    height: grid ? grid.cellHeight : 0
    iconSize: grid ? grid.iconSize : 48
    iconSource: row ? row.decoration : ""
    label: folderEntry ? (modelData.name || i18n("Folder")) : row ? row.display : ""
    subtitle: folderEntry ? i18np("%1 app", "%1 apps", modelData.apps.length) : ""
    badge: row ? row.isNewlyInstalled : false
    folderIcons: {
        launcherData.favoriteIds
        if (!folderEntry)
            return []
        return modelData.apps.slice(0, 4).map(index => {
            const item = launcherData.favoriteRow(index)
            return item ? item.decoration : ""
        })
    }
    game: folderEntry ? null : launcherData.gameForApp(favoriteId)
    selected: GridView.isCurrentItem && !!grid && grid.sectionActive
    reorderable: !!grid && grid.reorderable === true
    dropTarget: !!grid && grid.dropIndex === index && grid.dragIndex !== index && grid.dragIndex >= 0
    dropInto: !!grid && grid.dropInto

    function locate(position) {
        const mapped = tile.mapToItem(grid.contentItem, position.x, position.y)
        const target = grid.indexAt(mapped.x, mapped.y)
        const offset = ((mapped.x % grid.cellWidth) + grid.cellWidth) % grid.cellWidth / grid.cellWidth
        const outside = mapped.x < 0 || mapped.y < 0 || mapped.x > grid.width || mapped.y > grid.contentHeight
        return { target: target, into: !tile.folderEntry && !tile.insideFolder && offset > 0.22 && offset < 0.78, outside: outside }
    }
    onReorderMove: function(position) {
        const spot = locate(position)
        grid.dragIndex = index
        grid.dropIndex = spot.outside ? -1 : spot.target
        grid.dropInto = spot.into
    }
    onReorderDrop: function(position) {
        const spot = locate(position)
        grid.dragIndex = -1
        grid.dropIndex = -1
        grid.dropInto = false
        if (tile.insideFolder) {
            if (spot.outside || spot.target < 0)
                launcherData.removeFromFolder(favoriteId)
            return
        }
        if (spot.target >= 0 && spot.target !== index)
            launcher.pinDrop(grid.model, index, spot.target, spot.into)
    }

    function activate() {
        if (folderEntry)
            launcher.toggleFolder(modelData.id)
        else
            launcher.trigger(launcherData.favorites, modelData.favIndex, favoriteId)
    }
    function openMenu() {
        if (folderEntry)
            launcher.openMenu(launcher.folderEntries(modelData), tile)
        else
            launcher.openMenu(launcher.kickerEntries(launcherData.favorites, modelData.favIndex, row && row.hasActionList ? row.actionList : [], favoriteId), tile)
    }

    onHovered: launcher.select(grid, index)
    onClicked: activate()
    onRightClicked: {
        launcher.select(grid, index)
        openMenu()
    }
}
