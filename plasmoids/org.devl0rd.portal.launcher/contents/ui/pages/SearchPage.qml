import QtQuick
import QtQuick.Layouts
import QtQml.Models
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
    readonly property int rowWidth: Kirigami.Units.gridUnit * 19
    readonly property int groupCount: launcherData.runner.count
    readonly property bool showApps: mode === "all" || mode === "apps"
    readonly property var appGroup: showApps && groupCount > 0 ? launcherData.runner.modelForRow(0) : null
    readonly property bool appsAreApps: appGroup !== null && appGroup.count > 0
    readonly property var gameMatches: (mode === "all" || mode === "games") && term !== "" && launcherData.gamesEnabled
        ? launcherData.games.filter(game => Highlight.matches(game.name, term)).slice(0, mode === "games" ? 40 : 8) : []
    readonly property var friendMatches: (mode === "all" || mode === "friends") && term !== "" && launcherData.friendsEnabled
        ? launcherData.friends.filter(friend => Highlight.matches(friend.name, term) || Highlight.matches(friend.game, term)).slice(0, mode === "friends" ? 40 : 6) : []
    readonly property bool showPackages: (mode === "all" || mode === "packages") && launcherData.packagesEnabled
    readonly property bool heroIsGame: !appsAreApps && gameMatches.length > 0

    property var sections: []
    property int totalResults: 0

    function rebuildSections() {
        const list = [hero, heroGame]
        let total = 0
        const apps = firstGroup.count > 0 ? firstGroup.itemAt(0) : null
        if (apps)
            list.push(apps.grid)
        list.push(gamesGrid, packagesGrid, friendsGrid)
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
    Connections {
        target: launcherData
        function onPackagesChanged() { Qt.callLater(page.rebuildSections) }
    }

    component ResultGroup: ColumnLayout {
        id: group
        property string title
        property string trailing: resultGrid.count + ""
        property alias grid: resultGrid
        property alias model: resultGrid.model
        property alias delegate: resultGrid.delegate
        property alias cellHeight: resultGrid.cellHeight
        property bool busy: false
        visible: resultGrid.count > 0 || busy
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        SectionHeader {
            title: group.title
            trailing: group.busy && resultGrid.count === 0 ? i18n("searching…") : group.trailing
        }
        TileGrid {
            id: resultGrid
            visible: count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            cellWidth: Math.floor(width / Math.max(1, Math.floor(width / page.rowWidth)))
            cellHeight: Kirigami.Units.gridUnit * 3.2
            onCountChanged: Qt.callLater(page.rebuildSections)
        }
    }

    component HeroCard: Item {
        id: heroCard
        property var iconSource
        property string label
        property string subtitle
        property string kind
        property bool selected: false
        property var actions: []
        property Component art: null
        signal clicked()
        signal rightClicked()
        signal hovered()

        Rectangle {
            anchors.fill: parent
            anchors.margins: 2
            radius: Kirigami.Units.cornerRadius * 3
            color: heroCard.selected ? launcher.selectedFill : heroMouse.containsMouse ? launcher.hoverFill : launcher.well
            border.width: 1
            border.color: heroCard.selected ? launcher.selectedLine : launcher.hairline
        }
        MouseArea {
            id: heroMouse
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onEntered: heroCard.hovered()
            onClicked: function(event) {
                if (event.button === Qt.RightButton)
                    heroCard.rightClicked()
                else
                    heroCard.clicked()
            }
        }
        RowLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing * 1.5
            spacing: Kirigami.Units.largeSpacing * 2
            Loader {
                Layout.preferredWidth: heroCard.art ? Math.round(height / 0.4667) : Kirigami.Units.iconSizes.huge
                Layout.preferredHeight: heroCard.art ? parent.height : Kirigami.Units.iconSizes.huge
                sourceComponent: heroCard.art ? heroCard.art : heroIcon
                Component {
                    id: heroIcon
                    Kirigami.Icon {
                        source: heroCard.iconSource
                        fallback: "application-x-executable"
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing * 0.5
                PlasmaComponents.Label {
                    text: heroCard.kind
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    font.weight: Font.DemiBold
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 0.6
                    opacity: 0.5
                }
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    text: heroCard.label
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.6
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    visible: text !== ""
                    text: heroCard.subtitle
                    opacity: 0.6
                    elide: Text.ElideRight
                }
            }
            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                Repeater {
                    model: heroCard.actions
                    PlasmaComponents.Button {
                        required property var modelData
                        required property int index
                        text: modelData.text
                        icon.name: modelData.icon
                        highlighted: index === 0
                        onClicked: modelData.run()
                    }
                }
            }
        }
    }

    SectionHeader {
        visible: hero.visible || heroGame.visible
        title: i18n("Best match")
        trailing: ""
    }

    TileGrid {
        id: hero
        visible: page.appsAreApps && count > 0
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        limit: 1
        cellWidth: width
        cellHeight: Kirigami.Units.gridUnit * 5.4
        model: page.appsAreApps ? page.appGroup : null
        delegate: HeroCard {
            id: appHero
            required property int index
            required property var model
            readonly property var grid: GridView.view
            readonly property string favoriteId: model.favoriteId || ""
            width: grid.cellWidth
            height: grid.cellHeight
            iconSource: model.decoration
            label: model.display || ""
            subtitle: model.description || ""
            kind: page.appGroup ? page.appGroup.name : i18n("Application")
            selected: GridView.isCurrentItem && grid.sectionActive
            actions: {
                const list = [{ text: i18n("Open"), icon: "system-run", run: () => appHero.activate() }]
                if (favoriteId !== "")
                    list.push({ text: launcher.isPinned(favoriteId) ? i18n("Unpin") : i18n("Pin"), icon: "window-pin", run: () => launcher.togglePin(favoriteId) })
                list.push({ text: i18n("More"), icon: "overflow-menu", run: () => appHero.openMenu() })
                return list
            }
            function activate() {
                launcher.trigger(grid.model, index)
            }
            function openMenu() {
                launcher.openMenu(launcher.kickerEntries(grid.model, index, model.hasActionList ? model.actionList : [], favoriteId), appHero)
            }
            onHovered: launcher.select(grid, index)
            onClicked: activate()
            onRightClicked: {
                launcher.select(grid, index)
                openMenu()
            }
        }
        onCountChanged: Qt.callLater(page.rebuildSections)
    }

    TileGrid {
        id: heroGame
        visible: page.heroIsGame
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        limit: 1
        cellWidth: width
        cellHeight: Kirigami.Units.gridUnit * 6
        model: page.heroIsGame ? page.gameMatches.slice(0, 1) : []
        delegate: HeroCard {
            id: gameHero
            required property int index
            required property var modelData
            readonly property var grid: GridView.view
            readonly property var playing: launcherData.friendsFor(modelData)
            width: grid.cellWidth
            height: grid.cellHeight
            label: modelData.name
            subtitle: playing.length > 0 ? i18np("%1 friend playing now", "%1 friends playing now", playing.length)
                    : modelData.last > 0 ? i18n("Last played %1", Qt.formatDate(new Date(modelData.last * 1000), Qt.locale().dateFormat(Locale.ShortFormat))) : ""
            kind: i18n("Game")
            art: Component {
                GameArt {
                    game: gameHero.modelData
                    wide: true
                    radius: Kirigami.Units.cornerRadius * 2
                }
            }
            selected: GridView.isCurrentItem && grid.sectionActive
            actions: [
                { text: i18n("Play"), icon: "media-playback-start", run: () => gameHero.activate() },
                { text: i18n("More"), icon: "overflow-menu", run: () => gameHero.openMenu() }
            ]
            function activate() {
                launcherData.launchGame(modelData)
                root.hide()
            }
            function openMenu() {
                launcher.openMenu(launcher.gameEntries(modelData), gameHero)
            }
            onHovered: launcher.select(grid, index)
            onClicked: activate()
            onRightClicked: {
                launcher.select(grid, index)
                openMenu()
            }
        }
        onCountChanged: Qt.callLater(page.rebuildSections)
    }

    DelegateModel {
        id: otherApps
        model: page.appsAreApps ? page.appGroup : null
        groups: DelegateModelGroup {
            id: restGroup
            name: "rest"
            includeByDefault: true
        }
        filterOnGroup: "rest"
        items.onChanged: if (items.count > 0 && items.get(0).inRest) items.removeGroups(0, 1, "rest")
        delegate: KickerRow {
            sourceModel: page.appGroup
            sourceIndex: DelegateModel.itemsIndex
        }
    }

    Repeater {
        id: firstGroup
        model: page.showApps && page.groupCount > 0 ? 1 : 0
        onItemAdded: Qt.callLater(page.rebuildSections)
        delegate: ResultGroup {
            title: page.appGroup ? page.appGroup.name : ""
            model: otherApps
        }
    }

    ResultGroup {
        id: gamesResults
        title: i18n("Games")
        model: page.heroIsGame ? page.gameMatches.slice(1) : page.gameMatches
        delegate: RowTile {
            id: gameRow
            required property int index
            required property var modelData
            readonly property var grid: GridView.view
            readonly property var playing: launcherData.friendsFor(modelData)
            width: grid.cellWidth
            height: grid.cellHeight
            iconSource: modelData.icon || "applications-games"
            label: modelData.name
            query: page.term
            subtitle: playing.length > 0 ? i18np("%1 friend playing", "%1 friends playing", playing.length) : modelData.appid ? i18n("Steam game") : i18n("Game")
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
        id: packagesResults
        title: i18n("Install with Shelly")
        visible: page.showPackages && (grid.count > 0 || busy)
        busy: launcherData.packagesBusy
        model: page.showPackages ? launcherData.packages : []
        cellHeight: Kirigami.Units.gridUnit * 3.2
        delegate: RowTile {
            id: packageRow
            required property int index
            required property var modelData
            readonly property var grid: GridView.view
            width: grid.cellWidth
            height: grid.cellHeight
            iconSource: modelData.source === "aur" ? "package-x-generic-symbolic" : "package-symbolic"
            label: modelData.name
            query: page.term
            subtitle: modelData.description
            trailing: modelData.source === "aur" ? i18n("AUR · %1 votes", modelData.votes) : modelData.repo
            selected: GridView.isCurrentItem && grid.sectionActive
            function activate() {
                launcherData.installPackage(modelData)
                root.hide()
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
    readonly property alias packagesGrid: packagesResults.grid

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
        visible: page.totalResults === 0 && !launcherData.runner.querying && !launcherData.packagesBusy && page.term !== ""
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.gridUnit * 4
        iconName: "edit-find"
        text: i18n("No results for “%1”", page.term)
        explanation: i18n("Try fewer letters, or a prefix: g games · a apps · f files · s packages · @ friends · = math · > command")
    }

    Item {
        Layout.fillHeight: true
    }
}
