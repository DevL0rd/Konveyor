import QtQuick
import QtCore
import QtQuick.Dialogs
import org.kde.plasma.plasma5support as P5Support
import org.kde.plasma.private.kicker as Kicker
import org.kde.coreaddons as KCoreAddons

Item {
    id: launcherData

    required property var applet
    readonly property var config: applet.launcherConfig
    property bool live: false
    property string query: ""
    property string searchMode: "all"

    readonly property string portalBin: "$HOME/.local/bin/portal-games"
    readonly property bool gamesEnabled: launcherData.config.showGames
    readonly property bool friendsEnabled: launcherData.config.showFriends

    readonly property alias rootModel: rootModel
    readonly property alias favorites: rootModel.favoritesModel
    readonly property alias runner: runnerModel
    readonly property alias windows: windowSource
    readonly property alias system: systemModel
    readonly property alias recentApps: recentAppsModel
    readonly property alias recentDocs: recentDocsModel
    readonly property alias places: placesModel
    readonly property alias user: kuser
    readonly property url homeUrl: StandardPaths.writableLocation(StandardPaths.HomeLocation)

    property var games: []
    property var gameByDesktop: ({})
    property var recentGames: []
    property var friends: []
    property var friendsByAppid: ({})
    readonly property var playingNow: {
        const byGame = {}
        for (const friend of friends) {
            if (!friend.ingame)
                continue
            const key = friend.appid || friend.game
            if (!byGame[key]) {
                const owned = games.find(game => friend.appid && game.appid === friend.appid) || null
                byGame[key] = { key: key, appid: friend.appid || "", name: friend.game, header: friend.header || "", game: owned, friends: [] }
            }
            byGame[key].friends.push(friend)
        }
        return Object.values(byGame).sort((a, b) => b.friends.length - a.friends.length || String(a.name).localeCompare(String(b.name)))
    }
    property int friendsOnline: 0
    property int friendsInGame: 0
    property string friendsError: ""
    property bool friendsNeedsApiKey: false
    property bool steamKeyBusy: false
    property string steamKeyResult: ""
    property bool steamKeyError: false
    property real gamesLoadedAt: 0
    property var now: new Date()
    property var packages: []
    property string packagesQuery: ""
    property bool packagesBusy: false
    property int packagesRequest: 0
    readonly property bool packagesEnabled: launcherData.config.searchPackages

    Timer {
        running: launcherData.live
        repeat: true
        triggeredOnStart: true
        interval: 10000
        onTriggered: launcherData.now = new Date()
    }

    function shq(text) {
        return "'" + String(text).replace(/'/g, "'\\''") + "'"
    }

    KCoreAddons.KUser {
        id: kuser
    }

    Kicker.RootModel {
        id: rootModel
        autoPopulate: true
        appletInterface: launcherData.applet.kickerApplet
        flat: true
        sorted: true
        showSeparators: false
        showAllApps: true
        showAllAppsCategorized: false
        showRecentApps: false
        showRecentDocs: false
        showPowerSession: false
        highlightNewlyInstalledApps: true
        Component.onCompleted: favoritesModel.initForClient(launcherData.applet.favoritesClient)
    }

    readonly property var allRunners: {
        const list = ["krunner_services"]
        if (launcherData.config.searchSettings)
            list.push("krunner_systemsettings")
        if (launcherData.config.searchCalculator)
            list.push("calculator", "unitconverter")
        if (launcherData.config.searchCommands)
            list.push("krunner_shell")
        if (launcherData.config.searchFiles)
            list.push("krunner_placesrunner", "krunner_recentdocuments", "baloosearch", "locations")
        list.push("krunner_sessions", "krunner_powerdevil")
        if (launcherData.config.searchWeb)
            list.push("krunner_webshortcuts")
        return list
    }
    readonly property var modeRunners: ({
        all: allRunners,
        apps: ["krunner_services"],
        files: ["krunner_placesrunner", "krunner_recentdocuments", "baloosearch", "locations"],
        calc: ["calculator", "unitconverter"],
        command: ["krunner_shell"],
        games: [],
        friends: []
    })

    Kicker.RunnerModel {
        id: runnerModel
        appletInterface: launcherData.applet.kickerApplet
        favoritesModel: rootModel.favoritesModel
        mergeResults: false
        runners: launcherData.modeRunners[launcherData.searchMode] || launcherData.allRunners
        query: launcherData.live && (launcherData.modeRunners[launcherData.searchMode] || []).length > 0 ? launcherData.query : ""
    }

    WindowSource {
        id: windowSource
    }

    Kicker.SystemModel {
        id: systemModel
    }

    Kicker.RecentUsageModel {
        id: recentAppsModel
        shownItems: Kicker.RecentUsageModel.OnlyApps
    }
    property var recentRank: ({})
    property var popularRank: ({})
    readonly property bool popularWanted: live && launcherData.config.appsSort === "popular"
    Loader {
        active: launcherData.popularWanted
        sourceComponent: Item {
            Kicker.RecentUsageModel {
                id: popularModel
                shownItems: Kicker.RecentUsageModel.OnlyApps
                ordering: Kicker.RecentUsageModel.Popular
            }
            Instantiator {
                id: popularProbe
                model: popularModel
                delegate: QtObject {
                    required property var model
                    readonly property string favoriteId: model.favoriteId || ""
                }
                onObjectAdded: popularTimer.restart()
                onObjectRemoved: popularTimer.restart()
            }
            Timer {
                id: popularTimer
                interval: 0
                onTriggered: {
                    const rank = {}
                    for (let i = 0; i < popularProbe.count; ++i) {
                        const object = popularProbe.objectAt(i)
                        if (object && object.favoriteId)
                            rank[launcherData.desktopKey(object.favoriteId)] = i
                    }
                    launcherData.popularRank = rank
                }
            }
        }
    }
    Instantiator {
        id: recentProbe
        model: recentAppsModel
        delegate: QtObject {
            required property var model
            readonly property string favoriteId: model.favoriteId || ""
        }
        onObjectAdded: recentRankTimer.restart()
        onObjectRemoved: recentRankTimer.restart()
    }
    Timer {
        id: recentRankTimer
        interval: 0
        onTriggered: {
            const rank = {}
            for (let i = 0; i < recentProbe.count; ++i) {
                const object = recentProbe.objectAt(i)
                if (object && object.favoriteId)
                    rank[launcherData.desktopKey(object.favoriteId)] = i
            }
            launcherData.recentRank = rank
        }
    }

    Kicker.RecentUsageModel {
        id: recentDocsModel
        shownItems: Kicker.RecentUsageModel.OnlyDocs
    }

    Kicker.ComputerModel {
        id: placesModel
        appletInterface: launcherData.applet.kickerApplet
        systemApplications: []
    }

    P5Support.DataSource {
        id: runner
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
        }
    }
    function run(command) {
        runner.connectSource(command)
    }
    function addLauncher(place, app) {
        run("$HOME/.local/bin/portal-launcher add-to " + place + " " + shq(app))
    }

    P5Support.DataSource {
        id: gamesSource
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            if (result["exit code"] !== 0)
                return
            let parsed = null
            try {
                parsed = JSON.parse(result.stdout)
            } catch (error) {
                return
            }
            const list = parsed.games || []
            const byDesktop = {}
            for (const game of list)
                byDesktop[game.id] = game
            launcherData.gameByDesktop = byDesktop
            launcherData.games = list
            launcherData.recentGames = list.filter(game => game.last > 0).sort((a, b) => b.last - a.last).slice(0, 12)
            launcherData.gamesLoadedAt = Date.now()
        }
    }
    function refreshGames(force) {
        if (gamesEnabled && (force === true || Date.now() - gamesLoadedAt > 30000))
            gamesSource.connectSource(portalBin + " # " + Date.now())
    }

    P5Support.DataSource {
        id: artSource
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            launcherData.refreshGames(true)
        }
    }
    property var artGame: null
    Loader {
        id: artDialogLoader
        active: false
        sourceComponent: FileDialog {
            title: i18n("Choose game art")
            nameFilters: [i18n("Images (*.png *.jpg *.jpeg *.webp)")]
            onAccepted: {
                const path = decodeURIComponent(String(selectedFile).replace(/^file:\/\//, ""))
                if (launcherData.artGame)
                    artSource.connectSource(launcherData.portalBin + " --set-art " + launcherData.shq(launcherData.artGame.id) + " " + launcherData.shq(path))
                artDialogLoader.active = false
            }
            onRejected: artDialogLoader.active = false
        }
        onLoaded: item.open()
    }
    function pickArt(game) {
        artGame = game
        applet.hide()
        artDialogLoader.active = true
    }
    function resetArt(game) {
        artSource.connectSource(portalBin + " --reset-art " + shq(game.id))
    }

    TextEdit {
        id: clipboard
        visible: false
    }
    function copyText(text) {
        clipboard.text = text
        clipboard.selectAll()
        clipboard.copy()
        clipboard.text = ""
    }

    readonly property string packageTerm: {
        if (!live || !packagesEnabled)
            return ""
        const text = query.trim()
        if (!/[a-zA-Z]/.test(text))
            return ""
        if (searchMode === "packages")
            return text.length >= 2 ? text : ""
        if (searchMode !== "all")
            return ""
        return text.length >= 3 ? text : ""
    }
    onPackageTermChanged: {
        packagesRequest++
        if (packageTerm === "") {
            packagesDebounce.stop()
            packages = []
            packagesQuery = ""
            packagesBusy = false
            return
        }
        packagesBusy = true
        packagesDebounce.restart()
    }
    Timer {
        id: packagesDebounce
        interval: 450
        onTriggered: {
            const request = launcherData.packagesRequest
            packagesSource.connectSource("$HOME/.local/bin/portal-packages " + launcherData.shq(launcherData.packageTerm) + " " + (launcherData.searchMode === "packages" ? 30 : 6) + " # " + request)
        }
    }
    P5Support.DataSource {
        id: packagesSource
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            const request = parseInt(source.substring(source.lastIndexOf("# ") + 2))
            if (request !== launcherData.packagesRequest)
                return
            let parsed = null
            try {
                parsed = JSON.parse(result.stdout || "{}")
            } catch (error) {
                launcherData.packagesBusy = false
                return
            }
            launcherData.packages = parsed.packages || []
            launcherData.packagesQuery = parsed.query || ""
            launcherData.packagesBusy = false
        }
    }
    function installPackage(pkg) {
        run("konsole --hold -e shelly install " + (pkg.source === "aur" ? "aur" : "standard") + " " + shq(pkg.name))
    }
    function launchGame(game) {
        if (!game || !game.launch)
            return
        run(game.launch + " </dev/null >/dev/null 2>&1 & " + portalBin + " --track " + shq(game.id) + " # " + Date.now())
        const now = Date.now() / 1000
        const updated = games.map(entry => entry.id === game.id ? Object.assign({}, entry, { last: now }) : entry)
        games = updated
        recentGames = updated.filter(entry => entry.last > 0).sort((a, b) => b.last - a.last).slice(0, 12)
    }
    function desktopKey(favoriteId) {
        return String(favoriteId || "").replace(/^applications:/, "").replace(/\.desktop$/, "")
    }
    function gameForApp(favoriteId) {
        if (!favoriteId)
            return null
        return gameByDesktop[desktopKey(favoriteId)] || null
    }

    function steamGameForApp(favoriteId) {
        const game = gameForApp(favoriteId)
        return game && game.appid ? game : null
    }
}
