import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import "../lib"
import "../lib/Highlight.js" as Highlight
import ".."

PopScroll {
    id: page

    readonly property string term: launcher.term
    readonly property string mode: launcher.mode
    readonly property int rowWidth: Kirigami.Units.gridUnit * 17
    readonly property int groupCount: launcherData.runner.count
    readonly property bool showApps: mode === "all" || mode === "apps"
    readonly property var gameMatches: (mode === "all" || mode === "games") && term !== "" && launcherData.gamesEnabled
        ? launcherData.games.filter(game => Highlight.matches(game.name, term)).slice(0, mode === "games" ? 40 : 6) : []
    readonly property var friendMatches: (mode === "all" || mode === "friends") && term !== "" && launcherData.friendsEnabled
        ? launcherData.friends.filter(friend => Highlight.matches(friend.name, term) || Highlight.matches(friend.game, term)).slice(0, mode === "friends" ? 40 : 6) : []

    property var sections: []
    property int totalResults: 0

    function rebuildSections() {
        const list = []
        let total = 0
        const apps = firstGroup.count > 0 ? firstGroup.itemAt(0) : null
        if (apps)
            list.push(apps.grid)
        list.push(gamesGrid, friendsGrid)
        for (let i = 0; i < groups.count; ++i) {
            const item = groups.itemAt(i)
            if (item && !(page.showApps && i === 0))
                list.push(item.grid)
        }
        for (const grid of list)
            total += grid.visible ? grid.count : 0
        sections = list
        totalResults = total
        launcher.ensureSelection()
    }
    onTermChanged: Qt.callLater(rebuildSections)
    onGameMatchesChanged: Qt.callLater(rebuildSections)
    onFriendMatchesChanged: Qt.callLater(rebuildSections)

    component ResultGroup: ColumnLayout {
        id: group
        property string title
        property alias grid: resultGrid
        property alias model: resultGrid.model
        property alias delegate: resultGrid.delegate
        property alias cellHeight: resultGrid.cellHeight
        visible: resultGrid.count > 0
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        SectionHeader {
            title: group.title
            trailing: resultGrid.count + ""
        }
        TileGrid {
            id: resultGrid
            visible: count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            cellWidth: Math.floor(width / Math.max(1, Math.floor(width / page.rowWidth)))
            cellHeight: Kirigami.Units.gridUnit * 3
            onCountChanged: Qt.callLater(page.rebuildSections)
        }
    }

    Repeater {
        id: firstGroup
        model: page.showApps && page.groupCount > 0 ? 1 : 0
        onItemAdded: Qt.callLater(page.rebuildSections)
        delegate: ResultGroup {
            title: launcherData.runner.modelForRow(0) ? launcherData.runner.modelForRow(0).name : ""
            model: launcherData.runner.modelForRow(0)
            delegate: KickerRow {}
        }
    }

    ResultGroup {
        id: gamesResults
        title: i18n("Games")
        model: page.gameMatches
        delegate: RowTile {
            id: gameRow
            required property int index
            required property var modelData
            readonly property var grid: GridView.view
            width: grid.cellWidth
            height: grid.cellHeight
            iconSource: modelData.icon || "applications-games"
            label: modelData.name
            query: page.term
            subtitle: launcherData.friendsFor(modelData).length > 0 ? i18np("%1 friend playing", "%1 friends playing", launcherData.friendsFor(modelData).length) : modelData.appid ? i18n("Steam game") : i18n("Game")
            subtitleColor: launcherData.friendsFor(modelData).length > 0 ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
            selected: GridView.isCurrentItem && grid.sectionActive
            function activate() {
                launcherData.launchGame(modelData)
                root.hide()
            }
            function openMenu() {
                launcher.openMenu(launcher.gameEntries(modelData), gameRow)
            }
            onHovered: launcher.select(grid, index)
            onClicked: activate()
            onRightClicked: {
                launcher.select(grid, index)
                openMenu()
            }
        }
    }
    readonly property alias gamesGrid: gamesResults.grid

    ResultGroup {
        id: friendsResults
        title: i18n("Friends")
        cellHeight: Kirigami.Units.gridUnit * 3.2
        model: page.friendMatches
        delegate: FriendRow {}
    }
    readonly property alias friendsGrid: friendsResults.grid

    Repeater {
        id: groups
        model: page.groupCount
        delegate: ResultGroup {
            required property int index
            readonly property var runnerGroup: launcherData.runner.modelForRow(index)
            visible: grid.count > 0 && !(page.showApps && index === 0)
            title: runnerGroup ? runnerGroup.name : ""
            model: page.showApps && index === 0 ? null : runnerGroup
            delegate: KickerRow {}
        }
        onItemAdded: Qt.callLater(page.rebuildSections)
        onItemRemoved: Qt.callLater(page.rebuildSections)
    }

    PlasmaExtras.PlaceholderMessage {
        visible: page.totalResults === 0 && !launcherData.runner.querying && page.term !== ""
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.gridUnit * 4
        iconName: "edit-find"
        text: i18n("No results for “%1”", page.term)
        explanation: i18n("Try fewer letters, or a prefix: g games · f files · a apps · @ friends · = math · > command")
    }

    Item {
        Layout.fillHeight: true
    }
}
