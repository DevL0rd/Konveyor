import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import org.kde.plasma.private.kicker as Kicker
import "lib"

PlasmoidItem {
    id: root

    readonly property string panelIcon: Plasmoid.configuration.icon || "applications-all"
    Plasmoid.icon: panelIcon
    Plasmoid.title: i18n("App Portal")

    property var categories: []
    property string selectedLabel: "Favorites"
    function indexOfLabel(lbl) {
        for (let i = 0; i < categories.length; i++)
            if (categories[i].label === lbl) return i
        return 0
    }
    readonly property int currentIndex: indexOfLabel(selectedLabel)
    readonly property var currentCat: (currentIndex >= 0 && currentIndex < categories.length) ? categories[currentIndex] : null
    readonly property bool gamesActive: !!currentCat && currentCat.type === "games"
    readonly property bool favActive: !!currentCat && currentCat.type === "fav"
    readonly property bool appsActive: !!currentCat && currentCat.type === "apps"
    readonly property bool allAppsActive: appsActive && currentCat.allApps === true
    readonly property var appCategories: categories.filter(c => c.type === "apps")
    readonly property var appSectionCategories: categories.filter(c => c.type === "apps" && c.allApps !== true)
    readonly property var allAppsCategory: categories.find(c => c.type === "apps" && c.allApps === true) || null
    readonly property var gamesCategory: categories.find(c => c.type === "games") || null
    readonly property string tabKey: gamesActive ? "games" : appsActive ? "apps" : "fav"

    function selectCategory(label) {
        selectedLabel = label
        Plasmoid.configuration.defaultCategory = label
        const cat = categories[indexOfLabel(label)]
        if (cat && cat.type === "apps")
            Plasmoid.configuration.lastAppCategory = label
        Plasmoid.configuration.writeConfig()
    }
    function selectTab(key) {
        if (key === "fav" && categories.length > 0)
            selectCategory(categories[0].label)
        else if (key === "games" && gamesCategory)
            selectCategory(gamesCategory.label)
        else if (key === "apps" && appCategories.length > 0) {
            const last = Plasmoid.configuration.lastAppCategory
            selectCategory(appCategories.some(c => c.label === last) ? last : appCategories[0].label)
        }
    }

    property var favorites: []
    property var favSet: ({})
    function favKey(id) { return String(id || "").replace(/^applications:/, "") }

    property string sortMode: "recent"
    property string searchText: ""
    property var usage: ({})

    readonly property string portalBin: "$HOME/.local/bin/portal-games"
    function shq(s) { return "'" + String(s).replace(/'/g, "'\\''") + "'" }

    function appModelFor(cat) {
        if (!cat) return null
        if (cat.type === "apps") return rootModel.modelForRow(cat.row)
        return null
    }

    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical
    property bool popupAlive: !inPanel
    preferredRepresentation: inPanel ? compactRepresentation : fullRepresentation
    onExpandedChanged: function() {
        if (root.expanded) {
            releasePopup.stop()
            popupAlive = true
            loadUsage()
            loadFavorites()
            if (games.length === 0 && !gamesLoading)
                reloadGames()
            readFriends()
        } else if (inPanel) {
            releasePopup.restart()
        }
    }
    Timer {
        id: releasePopup
        interval: 1500
        onTriggered: root.popupAlive = root.expanded || !root.inPanel
    }

    P5Support.DataSource {
        id: favSrc
        engine: "executable"
        property string last
        onNewData: function(source, d) {
            disconnectSource(source)
            if (d.stdout === last)
                return
            last = d.stdout
            let list = []
            try { list = JSON.parse(d.stdout || "{}").favorites || [] } catch (e) {}
            root.favorites = list
            const s = {}
            for (const fav of list)
                s[root.favKey(fav.id)] = true
            root.favSet = s
        }
    }
    function loadFavorites() { favSrc.connectSource(portalBin + " --favorites") }
    onCurrentIndexChanged: if (favActive || allAppsActive) loadFavorites()
    Timer {
        interval: 2500
        repeat: true
        running: root.popupAlive && (root.favActive || root.allAppsActive)
        onTriggered: root.loadFavorites()
    }
    P5Support.DataSource {
        id: favWriter
        engine: "executable"
        onNewData: function(source, d) { disconnectSource(source); root.loadFavorites() }
    }
    function toggleFavorite(resource, add) {
        favWriter.connectSource(portalBin + (add ? " --fav-add " : " --fav-remove ") + shq(resource))
    }

    function zoom(step) {
        if (root.gamesActive)
            Plasmoid.configuration.gameCardWidth = Math.max(100, Math.min(320, Plasmoid.configuration.gameCardWidth + step * 12))
        else
            Plasmoid.configuration.iconSize = Math.max(32, Math.min(160, Plasmoid.configuration.iconSize + step * 8))
    }

    P5Support.DataSource {
        id: usageSrc
        engine: "executable"
        property string last
        onNewData: function(source, d) {
            disconnectSource(source)
            if (d.stdout === last)
                return
            last = d.stdout
            try { root.usage = JSON.parse(d.stdout || "{}") } catch (e) { root.usage = ({}) }
        }
    }
    P5Support.DataSource {
        id: runner
        engine: "executable"
        property bool reloadAfter: false
        onNewData: function(source, d) {
            disconnectSource(source)
            if (reloadAfter) {
                reloadAfter = false
                root.reloadGames()
            }
        }
    }
    function loadUsage() { usageSrc.connectSource(portalBin + " --usage") }
    function recordLaunch(key) { if (key) runner.connectSource(portalBin + " --track " + shq(key)) }
    function run(command) { if (command) runner.connectSource(command) }

    property var games: []
    property bool gamesLoading: false
    P5Support.DataSource {
        id: gamesSrc
        engine: "executable"
        onNewData: function(source, d) {
            disconnectSource(source)
            root.gamesLoading = false
            try { root.games = (JSON.parse(d.stdout || "{}").games) || [] } catch (e) { root.games = [] }
        }
    }
    function reloadGames() {
        gamesLoading = true
        gamesSrc.connectSource(portalBin)
    }
    function launchGame(g) {
        if (!g || !g.launch) return
        runner.connectSource(g.launch + " ; " + portalBin + " --track " + shq(g.id))
        launchAndClose()
    }
    function resetArt(g) {
        runner.reloadAfter = true
        runner.connectSource(portalBin + " --reset-art " + shq(g.id))
    }
    function setArt(g, url) {
        const path = decodeURIComponent(String(url).replace(/^file:\/\//, ""))
        runner.reloadAfter = true
        runner.connectSource(portalBin + " --set-art " + shq(g.id) + " " + shq(path))
    }
    function steamRun(url) { if (url) runner.connectSource("steam " + shq(url)) }

    property var friendsByAppid: ({})
    property int friendsPlaying: 0
    property string friendsPath: ""
    P5Support.DataSource {
        id: pathHelper
        engine: "executable"
        onNewData: function(source, d) {
            root.friendsPath = (d.stdout || "").trim()
            disconnectSource(source)
            root.readFriends()
        }
    }
    function readFriends() {
        if (!friendsPath) return
        const xhr = new XMLHttpRequest()
        xhr.open("GET", "file://" + friendsPath)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE) return
            let byAppid = {}
            try { byAppid = JSON.parse(xhr.responseText || "{}").by_appid || {} } catch (e) {}
            let playing = 0
            for (const appid in byAppid)
                playing += byAppid[appid].length
            root.friendsByAppid = byAppid
            root.friendsPlaying = playing
        }
        xhr.send()
    }
    function friendsFor(g) {
        return (g && g.appid && friendsByAppid[g.appid]) ? friendsByAppid[g.appid] : []
    }
    FileWatcher { path: root.friendsPath; onChanged: root.readFriends() }

    Component.onCompleted: {
        selectedLabel = Plasmoid.configuration.defaultCategory || "Favorites"
        sortMode = Plasmoid.configuration.defaultSort || "recent"
        appViewMode = Plasmoid.configuration.appViewMode || "grid"
        loadUsage()
        loadFavorites()
        pathHelper.connectSource("printf %s \"$XDG_RUNTIME_DIR/Plasma-App-Portal/friends.json\"")
    }

    Kicker.RootModel {
        id: rootModel
        autoPopulate: true
        appletInterface: root
        flat: false
        sorted: true
        showSeparators: false
        appNameFormat: 0
        showAllApps: true
        showAllAppsCategorized: false
        showRecentApps: false
        showRecentDocs: false
        showPowerSession: false
        onCountChanged: root.rebuildCategories()
        Component.onCompleted: root.rebuildCategories()
    }

    Instantiator {
        id: catRows
        model: rootModel
        delegate: QtObject {
            required property var model
            required property int index
            readonly property string label: model.display || ""
            readonly property int row: index
        }
        onObjectAdded: Qt.callLater(root.rebuildCategories)
        onObjectRemoved: Qt.callLater(root.rebuildCategories)
    }

    function rebuildCategories() {
        const cats = [{ label: i18n("Favorites"), type: "fav", row: -1 }]
        let hadGames = false
        let assignedAll = false
        for (let i = 0; i < catRows.count; i++) {
            const o = catRows.objectAt(i)
            if (!o || o.label === "") continue
            if (o.label.toLowerCase().indexOf("game") >= 0) {
                cats.push({ label: o.label, type: "games", row: o.row })
                hadGames = true
            } else {
                const isAll = !assignedAll
                if (isAll) assignedAll = true
                cats.push({ label: o.label, type: "apps", row: o.row, allApps: isAll })
            }
        }
        if (!hadGames)
            cats.push({ label: i18n("Games"), type: "games", row: -1 })
        root.categories = cats
    }

    function launchAndClose() {
        root.searchText = ""
        root.expanded = false
    }

    readonly property var sortOptions: [
        { id: "recent", label: i18n("Last opened"), icon: "view-history" },
        { id: "name", label: i18n("Name (A–Z)"), icon: "view-sort-ascending" },
        { id: "name_desc", label: i18n("Name (Z–A)"), icon: "view-sort-descending" }
    ]
    readonly property var gameViewOptions: [
        { id: "grid", label: i18n("Grid"), icon: "view-list-icons" },
        { id: "list", label: i18n("List"), icon: "view-list-details" },
        { id: "carousel", label: i18n("Shelf"), icon: "view-media-playlist" },
        { id: "carousel3d", label: i18n("Carousel"), icon: "view-presentation" },
        { id: "banner", label: i18n("Banners"), icon: "view-preview" }
    ]
    readonly property var appViewOptions: [
        { id: "grid", label: i18n("Grid"), icon: "view-list-icons" },
        { id: "list", label: i18n("List"), icon: "view-list-details" }
    ]
    property string appViewMode: "grid"
    readonly property var viewOptions: gamesActive ? gameViewOptions : appViewOptions
    readonly property string currentViewMode: gamesActive ? Plasmoid.configuration.gamesViewMode : appViewMode
    function setViewMode(id) {
        if (gamesActive) {
            Plasmoid.configuration.gamesViewMode = id
        } else {
            appViewMode = id
            Plasmoid.configuration.appViewMode = id
        }
    }
    function iconFor(opts, id, fallback) {
        for (const opt of opts)
            if (opt.id === id) return opt.icon
        return fallback
    }

    toolTipMainText: i18n("App Portal")
    toolTipSubText: {
        const parts = [i18np("%1 favourite", "%1 favourites", favorites.length)]
        if (games.length > 0)
            parts.push(i18np("%1 game", "%1 games", games.length))
        if (friendsPlaying > 0)
            parts.push(i18np("%1 friend in game", "%1 friends in game", friendsPlaying))
        return parts.join(" · ")
    }

    compactRepresentation: CompactView {}
    fullRepresentation: FullView {}
}
