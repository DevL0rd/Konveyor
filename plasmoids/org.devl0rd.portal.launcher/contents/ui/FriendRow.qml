import QtQuick
import org.kde.kirigami as Kirigami

RowTile {
    id: row

    required property int index
    required property var modelData
    readonly property var grid: GridView.view
    readonly property var friend: modelData

    width: grid.cellWidth
    height: grid.cellHeight
    roundIcon: true
    iconSource: friend.avatar || ""
    label: friend.name || ""
    query: launcher.searching ? launcher.term : ""
    subtitle: friend.ingame ? i18n("Playing %1", friend.game) : friend.state > 0 ? i18n("Online") : lastSeen()
    subtitleColor: friend.ingame ? Kirigami.Theme.positiveTextColor : friend.state > 0 ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
    ringColor: friend.ingame ? Kirigami.Theme.positiveTextColor : friend.state > 0 ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.2)
    selected: GridView.isCurrentItem && grid.sectionActive

    function lastSeen() {
        if (!friend.lastlogoff)
            return i18n("Offline")
        const minutes = Math.max(0, (Date.now() / 1000 - friend.lastlogoff) / 60)
        if (minutes < 60)
            return i18np("Last online %1 minute ago", "Last online %1 minutes ago", Math.round(minutes))
        if (minutes < 1440)
            return i18np("Last online %1 hour ago", "Last online %1 hours ago", Math.round(minutes / 60))
        return i18np("Last online %1 day ago", "Last online %1 days ago", Math.round(minutes / 1440))
    }
    function activate() {
        Qt.openUrlExternally(friend.chat)
        root.hide()
    }
    function openMenu() {
        launcher.openMenu(launcher.friendEntries(friend), row)
    }

    onHovered: launcher.select(grid, index)
    onClicked: activate()
    onRightClicked: {
        launcher.select(grid, index)
        openMenu()
    }
}
