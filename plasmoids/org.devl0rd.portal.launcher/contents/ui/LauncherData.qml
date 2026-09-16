import QtQuick
import org.kde.plasma.plasmoid
import org.kde.plasma.plasma5support as P5Support
import org.kde.plasma.private.kicker as Kicker
import org.kde.coreaddons as KCoreAddons
import "lib"

Item {
    id: data

    required property var applet
    property bool live: false
    property string query: ""
    property string searchMode: "all"

    readonly property string portalBin: "$HOME/.local/bin/portal-games"
    readonly property bool gamesEnabled: Plasmoid.configuration.showGames
    readonly property bool friendsEnabled: Plasmoid.configuration.showFriends

    readonly property alias rootModel: rootModel
    readonly property alias favorites: rootModel.favoritesModel
    readonly property alias runner: runnerModel
    readonly property alias system: systemModel
    readonly property alias recentApps: recentAppsModel
    readonly property alias recentDocs: recentDocsModel
    readonly property alias places: placesModel
    readonly property alias user: kuser

    property var games: []
    property var recentGames: []
    property var friends: []
    property var friendsByAppid: ({})
    property int friendsOnline: 0
    property int friendsInGame: 0
    property real gamesLoadedAt: 0

    function shq(text) {
        return "'" + String(text).replace(/'/g, "'\\''") + "'"
    }

    KCoreAddons.KUser {
        id: kuser
    }

    Kicker.RootModel {
        id: rootModel
        autoPopulate: true
        appletInterface: data.applet
        flat: true
        sorted: true
        showSeparators: false
        showAllApps: true
        showAllAppsCategorized: false
        showRecentApps: false
        showRecentDocs: false
        showPowerSession: false
        highlightNewlyInstalledApps: true
        Component.onCompleted: favoritesModel.initForClient("org.kde.plasma.kicker.favorites.instance-" + Plasmoid.id)
    }

    readonly property var allRunners: {
        const list = ["krunner_services"]
        if (Plasmoid.configuration.searchSettings)
            list.push("krunner_systemsettings")
        if (Plasmoid.configuration.searchCalculator)
            list.push("calculator", "unitconverter")
        if (Plasmoid.configuration.searchCommands)
            list.push("krunner_shell")
        if (Plasmoid.configuration.searchFiles)
            list.push("krunner_placesrunner", "krunner_recentdocuments", "baloosearch", "locations")
        list.push("krunner_sessions", "krunner_powerdevil")
        if (Plasmoid.configuration.searchWeb)
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
        appletInterface: data.applet
        favoritesModel: rootModel.favoritesModel
        mergeResults: false
        runners: data.modeRunners[data.searchMode] || data.allRunners
        query: data.live && (data.modeRunners[data.searchMode] || []).length > 0 ? data.query : ""
    }

    Kicker.SystemModel {
        id: systemModel
    }

    Kicker.RecentUsageModel {
        id: recentAppsModel
        shownItems: Kicker.RecentUsageModel.OnlyApps
    }

    Kicker.RecentUsageModel {
        id: recentDocsModel
        shownItems: Kicker.RecentUsageModel.OnlyDocs
    }

    Kicker.ComputerModel {
        id: placesModel
        appletInterface: data.applet
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

    P5Support.DataSource {
        id: gamesSource
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            let parsed = null
            try {
                parsed = JSON.parse(result.stdout || "{}")
            } catch (error) {
                return
            }
            const list = parsed.games || []
            data.games = list
            data.recentGames = list.filter(game => game.last > 0).sort((a, b) => b.last - a.last).slice(0, 12)
            data.gamesLoadedAt = Date.now()
        }
    }
    function refreshGames() {
        if (gamesEnabled && Date.now() - gamesLoadedAt > 30000)
            gamesSource.connectSource(portalBin)
    }
    function launchGame(game) {
        if (!game || !game.launch)
            return
        run(game.launch + " ; " + portalBin + " --track " + shq(game.id))
        const now = Date.now() / 1000
        const updated = games.map(entry => entry.id === game.id ? Object.assign({}, entry, { last: now }) : entry)
        games = updated
        recentGames = updated.filter(entry => entry.last > 0).sort((a, b) => b.last - a.last).slice(0, 12)
    }
    function friendsFor(game) {
        return game && game.appid && friendsByAppid[game.appid] ? friendsByAppid[game.appid] : []
    }

    property string friendsPath: ""
    P5Support.DataSource {
        id: pathSource
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            data.friendsPath = (result.stdout || "").trim()
            data.readFriends()
        }
    }
    function readFriends() {
        if (!friendsPath || !friendsEnabled)
            return
        const xhr = new XMLHttpRequest()
        xhr.open("GET", "file://" + friendsPath)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE || !xhr.responseText)
                return
            let parsed = null
            try {
                parsed = JSON.parse(xhr.responseText)
            } catch (error) {
                return
            }
            const list = (parsed.friends || []).slice().sort((a, b) => {
                const rank = f => f.ingame ? 0 : f.state > 0 ? 1 : 2
                const diff = rank(a) - rank(b)
                return diff !== 0 ? diff : String(a.name).toLowerCase().localeCompare(String(b.name).toLowerCase())
            })
            data.friends = list
            data.friendsByAppid = parsed.by_appid || {}
            data.friendsOnline = list.filter(f => f.state > 0).length
            data.friendsInGame = list.filter(f => f.ingame).length
        }
        xhr.send()
    }
    FileWatcher {
        path: data.live && data.friendsEnabled ? data.friendsPath : ""
        onChanged: data.readFriends()
    }

    onLiveChanged: {
        if (!live)
            return
        refreshGames()
        if (friendsPath === "")
            pathSource.connectSource("printf %s \"$XDG_RUNTIME_DIR/Plasma-App-Portal/friends.json\"")
        else
            readFriends()
    }
}
