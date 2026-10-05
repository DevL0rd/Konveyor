import QtQuick
import ".."

ResultGroup {
    title: i18n("Install with Shelly")
    visible: page.showPackages && grid.count > 0
    model: page.showPackages ? launcherData.packages : []
    delegate: RowTile {
        id: packageRow
        required property int index
        required property var modelData
        readonly property var grid: GridView.view
        width: grid.cellWidth
        height: grid.cellHeight
        iconSource: "package-x-generic-symbolic"
        monochrome: true
        label: modelData.name
        query: page.term
        subtitle: modelData.description
        trailing: modelData.source === "aur" ? i18np("AUR · %1 vote", "AUR · %1 votes", modelData.votes) : modelData.repo
        selected: GridView.isCurrentItem && grid.sectionActive
        function activate() {
            launcher.installPackage(modelData)
        }
        function openMenu() {
            launcher.openMenu(launcher.packageEntries(modelData), packageRow)
        }
        onHovered: launcher.select(grid, index)
        onClicked: activate()
        onRightClicked: {
            launcher.select(grid, index)
            openMenu()
        }
    }
}
