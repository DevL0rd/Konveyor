import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

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
    subtitleColor: friend.ingame ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
    ringColor: friend.ingame ? Kirigami.Theme.positiveTextColor : friend.state > 0 ? Qt.alpha(Kirigami.Theme.textColor, 0.85) : Qt.alpha(Kirigami.Theme.textColor, 0.18)
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

    PlasmaComponents.ToolButton {
        visible: row.containsMouse || row.selected || hovered
        icon.name: "dialog-messages"
        display: PlasmaComponents.AbstractButton.IconOnly
        text: i18n("Chat")
        onClicked: row.activate()
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.text: text
    }
    PlasmaComponents.ToolButton {
        visible: (row.containsMouse || row.selected || hovered) && !!row.friend.join && row.friend.ingame
        icon.name: "media-playback-start"
        display: PlasmaComponents.AbstractButton.IconOnly
        text: i18n("Join game")
        onClicked: {
            Qt.openUrlExternally(row.friend.join)
            root.hide()
        }
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.text: text
    }

    onHovered: launcher.select(grid, index)
    onClicked: activate()
    onRightClicked: {
        launcher.select(grid, index)
        openMenu()
    }
}
