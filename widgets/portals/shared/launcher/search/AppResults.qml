import QtQuick
import QtQml.Models
import ".."

ResultGroup {
    id: appsGroup
    function grids() {
        return [grid]
    }
    title: i18n("Applications")
    rows: shownRows.count
    model: DelegateModel {
        id: appRows
        model: page.appGroup
        groups: DelegateModelGroup {
            id: shownRows
            name: "shown"
            includeByDefault: false
        }
        filterOnGroup: "shown"
        items.onChanged: appsGroup.filterRows()
        delegate: KickerRow {
            sourceModel: page.appGroup
            sourceIndex: DelegateModel.itemsIndex
            position: DelegateModel.shownIndex
            function activate() {
                launcher.trigger(page.appGroup, DelegateModel.itemsIndex, favoriteId)
            }
        }
    }
    function filterRows() {
        const items = appRows.items
        const heroRow = page.hero.kind === "app" ? page.hero.row : page.hero.appRow !== undefined ? page.hero.appRow : -1
        for (let i = 0; i < items.count; ++i) {
            const entry = items.get(i)
            const id = entry.model.favoriteId || ""
            const keep = i !== heroRow && !launcherData.isHidden(id) && launcherData.steamGameForApp(id) === null
            if (keep && !entry.inShown)
                items.addGroups(i, 1, "shown")
            else if (!keep && entry.inShown)
                items.removeGroups(i, 1, "shown")
        }
    }
    Connections {
        target: page
        function onHeroChanged() { appsGroup.filterRows() }
    }
}
