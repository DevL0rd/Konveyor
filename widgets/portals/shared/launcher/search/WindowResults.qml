import QtQuick
import ".."

ResultGroup {
    id: windowsGroup
    function grids() {
        return [grid]
    }
    title: i18n("Open windows")
    model: page.windowMatches
    Component.onCompleted: page.windowSection = windowsGroup
    Component.onDestruction: if (page.windowSection === windowsGroup) page.windowSection = null
    delegate: RowTile {
        id: windowRow
        required property int index
        required property var modelData
        readonly property var grid: GridView.view
        width: grid.cellWidth
        height: grid.cellHeight
        iconSource: modelData.icon
        label: modelData.title
        query: page.term
        subtitle: modelData.minimized ? i18n("%1 · minimized", modelData.appName) : modelData.appName
        selected: GridView.isCurrentItem && grid.sectionActive
        function activate() {
            launcher.closeAndRun(() => launcherData.windows.activate(modelData))
        }
        function openMenu() {
            launcher.openMenu([
                { text: i18n("Switch to window"), icon: "window", run: () => windowRow.activate() },
                { text: i18n("Close window"), icon: "window-close", run: () => launcherData.windows.close(modelData) }
            ], windowRow)
        }
        onHovered: launcher.select(grid, index)
        onClicked: activate()
        onRightClicked: {
            launcher.select(grid, index)
            openMenu()
        }
    }
}
