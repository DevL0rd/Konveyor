import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQml.Models
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import "../lib/Highlight.js" as Highlight
import org.kde.konveyor.settings
import ".."
import "../search"

PopScroll {
    id: page

    readonly property string term: launcher.presentedTerm
    readonly property string mode: launcher.presentedMode
    readonly property int rowWidth: Kirigami.Units.gridUnit * 19
    readonly property int groupCount: launcherData.runner.count
    property int groupsRevision: 0

    readonly property var order: {
        const known = ["answer", "apps", "games", "windows", "settings", "files", "friends", "commands", "other"]
        const wanted = String(launcherData.config.searchOrder || "").split(",").map(key => key.trim()).filter(key => known.indexOf(key) >= 0)
        for (const key of known) {
            if (wanted.indexOf(key) < 0)
                wanted.push(key)
        }
        return wanted
    }

    function classify(name) {
        const text = String(name || "").toLowerCase()
        if (text.indexOf("application") >= 0)
            return "apps"
        if (text.indexOf("calculat") >= 0 || text.indexOf("unit conver") >= 0 || text.indexOf("date and time") >= 0 || text.indexOf("dictionary") >= 0)
            return "answer"
        if (text.indexOf("setting") >= 0)
            return "settings"
        if (text.indexOf("place") >= 0 || text.indexOf("recent") >= 0 || text.indexOf("file") >= 0 || text.indexOf("location") >= 0 || text.indexOf("desktop search") >= 0 || text.indexOf("document") >= 0)
            return "files"
        if (text.indexOf("command") >= 0 || text.indexOf("shell") >= 0 || text.indexOf("terminal") >= 0)
            return "commands"
        return "other"
    }
    readonly property var groupsByKind: {
        groupsRevision
        if (!launcher.searchSettled)
            return {}
        const kinds = {}
        for (let row = 0; row < groupCount; ++row) {
            const group = launcherData.runner.modelForRow(row)
            const kind = group ? classify(group.name) : "other"
            if (!kinds[kind])
                kinds[kind] = []
            kinds[kind].push(row)
        }
        return kinds
    }
    readonly property var appGroup: {
        const rows = groupsByKind.apps || []
        return rows.length > 0 && (mode === "all" || mode === "apps") ? launcherData.runner.modelForRow(rows[0]) : null
    }

    readonly property var gameMatches: (mode === "all" || mode === "games") && term !== "" && launcherData.gamesEnabled
        ? launcherData.games.filter(game => Highlight.matches(game.name, term)).map(game => {
            const at = String(game.name).toLowerCase().indexOf(term.toLowerCase())
            return { game: game, score: at < 0 ? 1000 : at }
        }).sort((a, b) => (a.score - b.score) || (b.game.last - a.game.last)).map(entry => entry.game).slice(0, mode === "games" ? 40 : 8) : []
    readonly property var friendMatches: (mode === "all" || mode === "friends") && term !== "" && launcherData.friendsEnabled
        ? launcherData.friends.filter(friend => Highlight.matches(friend.name, term) || Highlight.matches(friend.game, term)).slice(0, mode === "friends" ? 40 : 6) : []
    readonly property var windowMatches: mode === "all" && term !== "" && launcherData.config.searchWindows ? launcherData.windows.matches(term) : []
    onWindowMatchesChanged: Qt.callLater(rebuildSections)
    property Item windowSection: null
    readonly property bool showPackages: (mode === "all" || mode === "packages") && launcherData.packagesEnabled
    readonly property var settingMatches: mode === "all" && term.length >= 2 ? SettingsIndex.search(term).slice(0, 6) : []
    onSettingMatchesChanged: Qt.callLater(rebuildSections)
    readonly property var shortcutMatches: mode === "all" && term.length >= 2 ? launcherData.shortcutMatches(term, 6) : []
    onShortcutMatchesChanged: Qt.callLater(rebuildSections)

    property var appIds: []
    function collectApps() {
        const ids = []
        const source = appsProbe
        for (let i = 0; i < source.count; ++i) {
            const object = source.objectAt(i)
            ids.push(object ? object.favoriteId : "")
        }
        appIds = ids
    }
    Instantiator {
        id: appsProbe
        model: page.appGroup
        delegate: QtObject {
            required property var model
            readonly property string favoriteId: model.favoriteId || ""
        }
        onObjectAdded: Qt.callLater(page.collectApps)
        onObjectRemoved: Qt.callLater(page.collectApps)
    }
    onAppGroupChanged: Qt.callLater(collectApps)

    readonly property var hero: {
        appIds
        const learned = launcherData.learnedFor(term)
        for (const key of learned) {
            if (key.startsWith("game:")) {
                const game = gameMatches.find(entry => "game:" + entry.id === key)
                if (game)
                    return { kind: "game", game: game }
            } else {
                const row = appIds.indexOf(key)
                if (row >= 0 && !launcherData.isHidden(key)) {
                    const game = launcherData.steamGameForApp(key)
                    return game ? { kind: "game", game: game, appRow: row } : { kind: "app", row: row }
                }
            }
        }
        for (let row = 0; row < appIds.length; ++row) {
            if (launcherData.isHidden(appIds[row]))
                continue
            const game = launcherData.steamGameForApp(appIds[row])
            return game ? { kind: "game", game: game, appRow: row } : { kind: "app", row: row }
        }
        if (gameMatches.length > 0)
            return { kind: "game", game: gameMatches[0] }
        return { kind: "none" }
    }
    readonly property var gameRows: {
        const heroId = hero.kind === "game" ? hero.game.id : ""
        const list = gameMatches.filter(game => game.id !== heroId && (game.appid !== "" || appIds.every(id => launcherData.desktopKey(id) !== game.id)))
        for (const id of appIds) {
            const game = launcherData.steamGameForApp(id)
            if (game && game.id !== heroId && !list.some(entry => entry.id === game.id))
                list.push(game)
        }
        return list
    }

    property var sections: []
    property int totalResults: 0

    function rebuildSections() {
        const list = [heroApp, heroGame]
        for (let i = 0; i < slots.count; ++i) {
            const slot = slots.itemAt(i)
            if (slot && slot.item && slot.item.hasContent)
                list.push.apply(list, slot.item.grids ? slot.item.grids() : [])
        }
        list.push(shortcutResults.grid)
        list.push(packagesResults.grid)
        let total = 0
        for (const grid of list)
            total += grid && grid.visible ? grid.count : 0
        sections = list
        totalResults = total
        launcher.ensureSelection()
    }
    onTermChanged: {
        if (term !== "")
            launcherData.ensureShortcuts()
        Qt.callLater(rebuildSections)
    }
    onHeroChanged: {
        pickHeroRow()
        Qt.callLater(rebuildSections)
    }
    onGameRowsChanged: Qt.callLater(rebuildSections)
    onFriendMatchesChanged: Qt.callLater(rebuildSections)
    onGroupCountChanged: {
        groupsRevision++
        Qt.callLater(rebuildSections)
    }
    Connections {
        target: launcherData
        function onPackagesChanged() { Qt.callLater(page.rebuildSections) }
    }
    Connections {
        target: launcherData.runner
        function onQueryFinished() {
            page.groupsRevision++
            Qt.callLater(page.rebuildSections)
        }
    }

    SectionHeader {
        visible: heroApp.visible || heroGame.visible
        title: i18n("Best match")
        trailing: launcherData.learnedFor(page.term).length > 0 ? i18n("based on what you open") : ""
    }

    DelegateModel {
        id: heroAppModel
        model: page.hero.kind === "app" ? page.appGroup : null
        groups: DelegateModelGroup {
            name: "hero"
            includeByDefault: false
        }
        filterOnGroup: "hero"
        items.onChanged: page.pickHeroRow()
        delegate: AppHero {}
    }
    function pickHeroRow() {
        const items = heroAppModel.items
        const want = page.hero.kind === "app" ? page.hero.row : -1
        for (let i = 0; i < items.count; ++i) {
            const inHero = items.get(i).inHero
            if (i === want && !inHero)
                items.addGroups(i, 1, "hero")
            else if (i !== want && inHero)
                items.removeGroups(i, 1, "hero")
        }
    }

    TileGrid {
        id: heroApp
        visible: page.hero.kind === "app" && count > 0
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        limit: 1
        cellWidth: width
        cellHeight: Kirigami.Units.gridUnit * 5.4
        model: heroAppModel
        onCountChanged: Qt.callLater(page.rebuildSections)
    }

    TileGrid {
        id: heroGame
        visible: page.hero.kind === "game"
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        limit: 1
        cellWidth: width
        cellHeight: Kirigami.Units.gridUnit * 6
        model: page.hero.kind === "game" ? [page.hero.game] : []
        delegate: GameHero {}
        onCountChanged: Qt.callLater(page.rebuildSections)
    }

    Component {
        id: appsSlot
        AppResults {}
    }

    Component {
        id: gamesSlot
        GameResults {}
    }

    Component {
        id: friendsSlot
        ResultGroup {
            function grids() {
                return [grid]
            }
            title: i18n("Friends")
            model: page.friendMatches
            delegate: FriendRow {}
        }
    }

    Component {
        id: windowsSlot
        WindowResults {}
    }

    Component {
        id: runnerSlot
        RunnerResults {}
    }

    Repeater {
        id: slots
        model: page.order
        delegate: Loader {
            id: slot
            required property string modelData
            Layout.fillWidth: true
            Layout.preferredHeight: item && item.hasContent ? item.implicitHeight : 0
            visible: item !== null && item.hasContent
            active: modelData !== "apps" || page.appGroup !== null
            sourceComponent: modelData === "apps" ? appsSlot : modelData === "games" ? gamesSlot : modelData === "friends" ? friendsSlot : modelData === "windows" ? windowsSlot : runnerSlot
            onLoaded: {
                item.width = Qt.binding(() => slot.width)
                if (sourceComponent === runnerSlot)
                    item.kind = Qt.binding(() => modelData)
                Qt.callLater(page.rebuildSections)
            }
        }
    }

    ShortcutResults {
        id: shortcutResults
    }

    PackageResults {
        id: packagesResults
    }

    RowLayout {
        visible: page.totalResults === 0 && page.term !== "" && (launcherData.runner.querying || (launcherData.packagesBusy && page.showPackages))
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.gridUnit * 2
        spacing: Kirigami.Units.largeSpacing
        Kirigami.Icon {
            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
            Layout.preferredHeight: Layout.preferredWidth
            source: "search-symbolic"
            color: launcher.ink
            isMask: true
            opacity: 0.4
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            text: launcherData.packagesBusy && page.showPackages ? i18n("Nothing here yet — still checking files and Shelly packages…") : i18n("Searching…")
            opacity: 0.55
        }
    }

    NoResults {}

    Item {
        Layout.fillHeight: true
    }
}
