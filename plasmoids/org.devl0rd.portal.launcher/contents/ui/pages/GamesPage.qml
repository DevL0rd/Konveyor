import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import ".."

ColumnLayout {
    id: page

    readonly property var sections: [grid]
    property string filter: "all"
    property string sort: "recent"
    readonly property var filters: [
        { key: "all", label: i18n("All") },
        { key: "played", label: i18n("Played") },
        { key: "friends", label: i18n("Friends playing") }
    ]
    readonly property var shown: {
        let list = launcherData.games
        if (filter === "played")
            list = list.filter(game => game.last > 0)
        else if (filter === "friends")
            list = list.filter(game => game.appid && launcherData.friendsByAppid[game.appid])
        list = list.slice()
        if (sort === "name")
            list.sort((a, b) => String(a.name).toLowerCase().localeCompare(String(b.name).toLowerCase()))
        else
            list.sort((a, b) => (b.last - a.last) || String(a.name).toLowerCase().localeCompare(String(b.name).toLowerCase()))
        return list
    }

    spacing: Kirigami.Units.largeSpacing

    function cycle(forward) {
        const at = filters.findIndex(entry => entry.key === filter)
        filter = filters[(at + (forward ? 1 : -1) + filters.length) % filters.length].key
        Qt.callLater(launcher.resetSelection)
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing

        PopTabs {
            Layout.fillWidth: false
            Layout.preferredWidth: Kirigami.Units.gridUnit * 20
            model: page.filters.map(entry => ({ label: entry.label, badge: entry.key === "friends" && launcherData.friendsInGame > 0 ? launcherData.friendsInGame + "" : "" }))
            currentIndex: page.filters.findIndex(entry => entry.key === page.filter)
            onActivated: index => {
                page.filter = page.filters[index].key
                Qt.callLater(launcher.resetSelection)
            }
        }
        Item { Layout.fillWidth: true }
        PlasmaComponents.Label {
            text: i18np("%1 game", "%1 games", page.shown.length)
            opacity: 0.6
        }
        PlasmaComponents.ToolButton {
            icon.name: page.sort === "name" ? "view-sort-ascending" : "view-history"
            text: page.sort === "name" ? i18n("Name") : i18n("Last played")
            onClicked: {
                page.sort = page.sort === "name" ? "recent" : "name"
                Qt.callLater(launcher.resetSelection)
            }
        }
    }

    TileGrid {
        id: grid
        Layout.fillWidth: true
        Layout.fillHeight: true
        scrolling: true
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / (Kirigami.Units.gridUnit * 9.5))))
        cellHeight: Math.round(cellWidth * 1.5)
        model: page.shown
        delegate: GameCard {}
        QQC2.ScrollBar.vertical: PlasmaComponents.ScrollBar {}

        PlasmaComponents.Label {
            anchors.centerIn: parent
            visible: grid.count === 0
            text: page.filter === "friends" ? i18n("No friends are playing your games right now") : i18n("No games found")
            opacity: 0.6
        }
    }
}
