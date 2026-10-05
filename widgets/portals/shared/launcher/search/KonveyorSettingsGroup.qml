import QtQuick
import org.kde.konveyor.settings
import ".."

ResultGroup {
    title: i18n("Konveyor settings")
    delegate: RowTile {
        required property int index
        required property var modelData
        readonly property var grid: GridView.view
        readonly property var settingsPage: Pages.byId(modelData.page)
        width: grid.cellWidth
        height: grid.cellHeight
        iconSource: settingsPage ? settingsPage.icon : "configure-symbolic"
        label: modelData.label
        query: page.term
        subtitle: settingsPage ? settingsPage.title + (modelData.section ? " · " + modelData.section : "") : ""
        selected: GridView.isCurrentItem && grid.sectionActive
        function activate() {
            launcherData.settingsTarget = { page: modelData.page, section: modelData.section || "", label: modelData.label }
            launcher.goToPage("settings")
        }
        function openMenu() {
            activate()
        }
        onHovered: launcher.select(grid, index)
        onClicked: activate()
    }
}
