import QtQuick

RowTile {
    id: tile

    required property int index
    required property var model
    readonly property var grid: GridView.view
    readonly property string favoriteId: model.favoriteId || ""

    width: grid.cellWidth
    height: grid.cellHeight
    iconSource: model.decoration
    label: model.display || ""
    query: launcher.searching ? launcher.term : ""
    subtitle: model.description || ""
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
