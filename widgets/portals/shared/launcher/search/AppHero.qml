import QtQuick
import QtQml.Models

HeroCard {
    id: appHero
    required property var model
    readonly property int sourceRow: DelegateModel.itemsIndex
    readonly property var grid: GridView.view
    readonly property string favoriteId: model.favoriteId || ""
    readonly property var sidebarEntry: launcherData.sidebarEntryFor(favoriteId, "", model.display)
    width: grid ? grid.cellWidth : 0
    height: grid ? grid.cellHeight : 0
    iconSource: model.decoration
    label: model.display || ""
    subtitle: model.description || ""
    kind: i18n("Application")
    selected: GridView.isCurrentItem && !!grid && grid.sectionActive
    actions: {
        const list = [{ text: i18n("Open"), icon: "system-run", run: () => appHero.activate() }]
        if (favoriteId !== "")
            list.push({ text: launcher.isPinned(favoriteId) ? i18n("Unpin") : i18n("Pin"), icon: "window-pin", run: () => launcher.togglePin(favoriteId) })
        list.push({ text: i18n("More"), icon: "overflow-menu", run: () => appHero.openMenu() })
        return list
    }
    function activate() {
        launcher.trigger(page.appGroup, sourceRow, favoriteId)
    }
    function openMenu() {
        launcher.openMenu(launcher.kickerEntries(page.appGroup, sourceRow, model.hasActionList ? model.actionList : [], favoriteId), appHero)
    }
    onHovered: launcher.select(grid, 0)
    onClicked: activate()
    onRightClicked: {
        launcher.select(grid, 0)
        openMenu()
    }
}
