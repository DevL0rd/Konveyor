import QtQuick
import ".."

ResultGroup {
    title: i18n("Shortcuts")
    model: page.shortcutMatches
    delegate: RowTile {
        id: shortcutRow
        required property int index
        required property var modelData
        readonly property var grid: GridView.view
        width: grid.cellWidth
        height: grid.cellHeight
        iconSource: "input-keyboard-symbolic"
        monochrome: true
        label: modelData.action
        query: page.term
        subtitle: modelData.section
        trailing: launcherData.keyText(modelData.keys[0])
        selected: GridView.isCurrentItem && grid.sectionActive
        function activate() {
            launcherData.shortcutFocus = modelData.action
            launcher.goToPage("shortcuts")
        }
        function openMenu() {
            activate()
        }
        onHovered: launcher.select(grid, index)
        onClicked: activate()
    }
}
