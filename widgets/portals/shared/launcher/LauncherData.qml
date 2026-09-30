import QtQuick
import QtCore
import QtQuick.Dialogs
import org.kde.plasma.plasma5support as P5Support
import org.kde.plasma.private.kicker as Kicker
import org.kde.coreaddons as KCoreAddons
import "lib"

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
    property var playingNow: []
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
        if (launcherData.config.searchWindows)
            list.push("windows")
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

    property var shortcuts: []
    property string shortcutCategory: i18n("All")
    property string shortcutFocus: ""
    P5Support.DataSource {
        id: shortcutsSource
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            if (result["exit code"] !== 0) {
                const lines = (result.stderr || "").trim().split("\n")
                launcherData.shortcutsError = lines[lines.length - 1] || i18n("konveyor-cheatsheet exited with code %1", result["exit code"])
                return
            }
            try {
                launcherData.shortcuts = JSON.parse(result.stdout)
                launcherData.shortcutsError = ""
            } catch (error) {
                launcherData.shortcutsError = error.message
            }
        }
    }
    property string shortcutsError: ""
    property bool shortcutsRequested: false
    function refreshShortcuts() {
        shortcutsRequested = true
        shortcutsSource.connectSource("PATH=\"$HOME/.local/bin:$PATH\" konveyor-cheatsheet --json # " + Date.now())
    }
    function ensureShortcuts() {
        if (!shortcutsRequested)
            refreshShortcuts()
    }
    function keyText(key) {
        const names = { super: "Meta", mod: "Meta", page_down: "PgDn", page_up: "PgUp", bracketleft: "[", bracketright: "]", comma: ",", period: ".", minus: "−", equal: "=", return: "Enter" }
        return key.split("+").map(part => names[part.toLowerCase()] || part).join(" + ")
    }
    function shortcutMatches(term, limit) {
        const query = term.toLowerCase()
        const found = []
        for (const section of shortcuts) {
            for (const entry of section.entries) {
                if (entry.action.toLowerCase().includes(query) || entry.keys.join(" ").toLowerCase().includes(query))
                    found.push({ action: entry.action, keys: entry.keys, id: entry.id || "", section: section.name })
                if (found.length >= limit)
                    return found
            }
        }
        return found
    }
    property var settingsTarget: ({ page: "layout", section: "", label: "" })

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

    readonly property string statePath: String(StandardPaths.writableLocation(StandardPaths.GenericDataLocation)).replace(/^file:\/\//, "") + "/Plasma-App-Portal"
    property var hiddenList: []
    readonly property var hiddenSet: {
        const set = {}
        for (const id of hiddenList)
            set[desktopKey(id)] = true
        return set
    }
    function isHidden(favoriteId) {
        return !!favoriteId && hiddenSet[desktopKey(favoriteId)] === true
    }
    function setHidden(favoriteId, hidden) {
        const key = desktopKey(favoriteId)
        hiddenList = hidden ? hiddenList.filter(id => id !== key).concat([key]) : hiddenList.filter(id => id !== key)
        run(portalBin + (hidden ? " --hide " : " --unhide ") + shq(key))
    }
    function readHidden() {
        const xhr = new XMLHttpRequest()
        xhr.open("GET", "file://" + statePath + "/hidden.json")
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            let parsed = []
            try {
                parsed = JSON.parse(xhr.responseText || "[]")
            } catch (error) {
                return
            }
            launcherData.hiddenList = Array.isArray(parsed) ? parsed : []
        }
        xhr.send()
    }
    FileWatcher {
        path: launcherData.statePath + "/hidden.json"
        onChanged: launcherData.readHidden()
    }
    Component.onCompleted: {
        readHidden()
        readFolders()
    }

    property var sidebarPins: []
    property int sidebarWrites: 0
    property string sidebarQueued: ""
    P5Support.DataSource {
        id: sidebarSource
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            if (launcherData.sidebarWrites > 0)
                return
            let parsed = []
            try {
                parsed = JSON.parse(result.stdout || "[]")
            } catch (error) {
                return
            }
            const list = Array.isArray(parsed) ? parsed : []
            if (JSON.stringify(list) !== JSON.stringify(launcherData.sidebarPins))
                launcherData.sidebarPins = list
        }
    }
    P5Support.DataSource {
        id: sidebarWriter
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            if (launcherData.sidebarQueued !== "") {
                const next = launcherData.sidebarQueued
                launcherData.sidebarQueued = ""
                connectSource(next)
                return
            }
            launcherData.sidebarWrites = 0
            launcherData.readSidebar()
        }
    }
    function readSidebar() {
        sidebarSource.connectSource(portalBin + " --sidebar # " + Date.now())
    }
    FileWatcher {
        path: launcherData.statePath + "/sidebar.json"
        onChanged: if (launcherData.sidebarWrites === 0) launcherData.readSidebar()
    }
    function sidebarKey(pin) {
        return pin ? pin.kind + ":" + pin.id : ""
    }
    function setSidebar(list) {
        sidebarPins = list
        const command = portalBin + " --sidebar-set " + shq(JSON.stringify(list.map(pin => ({ kind: pin.kind, id: pin.id, name: pin.name || "" })))) + " # " + Date.now()
        if (sidebarWrites > 0) {
            sidebarQueued = command
            return
        }
        sidebarWrites = 1
        sidebarWriter.connectSource(command)
    }
    function sidebarEntryFor(favoriteId, url, name, icon) {
        const id = String(favoriteId || "")
        const link = String(url || "")
        const label = String(name || "")
        const iconName = typeof icon === "string" ? icon : ""
        if (id.startsWith("applications:") || id.endsWith(".desktop"))
            return { kind: "app", id: desktopKey(id), name: label, icon: iconName }
        const path = link.startsWith("file://") ? link : id.startsWith("file://") ? id : id.startsWith("/") ? "file://" + encodeURI(id) : ""
        if (path === "")
            return null
        return { kind: "path", id: path.replace(/(.)\/$/, "$1"), name: label, icon: iconName }
    }
    function sidebarEntryForGame(game) {
        return game && game.id ? { kind: "app", id: game.id, name: game.name || "", icon: game.icon || "" } : null
    }
    function sidebarIndex(entry) {
        const key = sidebarKey(entry)
        return key === "" ? -1 : sidebarPins.findIndex(pin => sidebarKey(pin) === key)
    }
    function isOnSidebar(entry) {
        return sidebarIndex(entry) >= 0
    }
    function addSidebar(entry, at) {
        if (!entry)
            return
        const list = sidebarPins.filter(pin => sidebarKey(pin) !== sidebarKey(entry))
        const index = at === undefined || at < 0 ? list.length : Math.min(at, list.length)
        list.splice(index, 0, Object.assign({ missing: false }, entry))
        setSidebar(list)
    }
    function removeSidebar(entry) {
        const key = sidebarKey(entry)
        setSidebar(sidebarPins.filter(pin => sidebarKey(pin) !== key))
    }
    function toggleSidebar(entry) {
        if (isOnSidebar(entry))
            removeSidebar(entry)
        else
            addSidebar(entry)
    }
    function moveSidebar(from, to) {
        const target = Math.max(0, Math.min(to, sidebarPins.length - 1))
        if (from < 0 || from >= sidebarPins.length || from === target)
            return
        const list = sidebarPins.slice()
        const moved = list.splice(from, 1)[0]
        list.splice(target, 0, moved)
        setSidebar(list)
    }
    function sidebarGame(pin) {
        return pin && pin.kind === "app" ? gameByDesktop[pin.id] || null : null
    }
    function openSidebarPin(pin) {
        if (!pin || pin.missing)
            return false
        if (pin.kind === "path") {
            Qt.openUrlExternally(pin.id)
            return true
        }
        const game = sidebarGame(pin)
        if (game && game.launch) {
            launchGame(game)
            return true
        }
        run("kstart --application " + shq(pin.id))
        trackApp(pin.id)
        return true
    }
    function showSidebarPinInFolder(pin) {
        run("dbus-send --session --type=method_call --dest=org.freedesktop.FileManager1 /org/freedesktop/FileManager1 org.freedesktop.FileManager1.ShowItems array:string:" + shq(pin.id) + " string:")
    }

    property var folders: []
    property var favoriteIds: []
    property var pinnedEntries: []
    property string pinnedSignature: ""
    Instantiator {
        id: favoriteRows
        model: launcherData.favorites
        delegate: QtObject {
            required property var model
            required property int index
            readonly property string favoriteId: model.favoriteId || ""
            readonly property var decoration: model.decoration
            readonly property string display: model.display || ""
            readonly property bool isNewlyInstalled: model.isNewlyInstalled === true
            readonly property bool hasActionList: model.hasActionList === true
            readonly property var actionList: model.actionList
        }
        onObjectAdded: Qt.callLater(launcherData.rebuildPinned)
        onObjectRemoved: Qt.callLater(launcherData.rebuildPinned)
    }
    Connections {
        target: launcherData.favorites
        function onRowsMoved() { Qt.callLater(launcherData.rebuildPinned) }
        function onModelReset() { Qt.callLater(launcherData.rebuildPinned) }
        function onDataChanged() { Qt.callLater(launcherData.rebuildPinned) }
    }
    onFoldersChanged: Qt.callLater(rebuildPinned)
    function favoriteRow(index) {
        return favoriteRows.objectAt(index)
    }
    function rebuildPinned() {
        const ids = []
        for (let row = 0; row < favoriteRows.count; ++row) {
            const item = favoriteRows.objectAt(row)
            ids.push(item ? item.favoriteId : "")
        }
        const folderOf = {}
        for (const folder of folders) {
            for (const app of folder.apps)
                folderOf[app] = folder
        }
        const entries = []
        const emitted = {}
        for (let row = 0; row < ids.length; ++row) {
            const folder = folderOf[ids[row]]
            if (!folder) {
                entries.push({ kind: "app", favIndex: row, favoriteId: ids[row] })
                continue
            }
            if (emitted[folder.id] !== undefined) {
                entries[emitted[folder.id]].apps.push(row)
                continue
            }
            emitted[folder.id] = entries.length
            entries.push({ kind: "folder", id: folder.id, name: folder.name, apps: [row], favIndex: row, favoriteId: "" })
        }
        const signature = JSON.stringify(entries)
        favoriteIds = ids
        if (signature !== pinnedSignature) {
            pinnedSignature = signature
            pinnedEntries = entries
        }
    }
    function folderById(id) {
        return folders.find(folder => folder.id === id) || null
    }
    function folderFor(favoriteId) {
        return folders.find(folder => folder.apps.indexOf(favoriteId) >= 0) || null
    }
    function readFolders() {
        const xhr = new XMLHttpRequest()
        xhr.open("GET", "file://" + statePath + "/folders.json")
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            let parsed = []
            try {
                parsed = JSON.parse(xhr.responseText || "[]")
            } catch (error) {
                return
            }
            const list = Array.isArray(parsed) ? parsed.filter(entry => entry && entry.id && Array.isArray(entry.apps)) : []
            if (JSON.stringify(list) !== JSON.stringify(launcherData.folders))
                launcherData.folders = list
        }
        xhr.send()
    }
    FileWatcher {
        path: launcherData.statePath + "/folders.json"
        onChanged: launcherData.readFolders()
    }
    function withoutApps(list, apps) {
        return list.map(folder => ({ id: folder.id, name: folder.name, apps: folder.apps.filter(app => apps.indexOf(app) < 0) })).filter(folder => folder.apps.length > 0)
    }
    function createFolder(apps, name) {
        const id = "folder-" + Date.now()
        const label = name || i18n("Folder")
        folders = withoutApps(folders, apps).concat([{ id: id, name: label, apps: apps.slice() }])
        run(portalBin + " --folder-create " + shq(id) + " " + shq(label) + " " + apps.map(app => shq(app)).join(" "))
        return id
    }
    function addToFolder(id, app) {
        const next = withoutApps(folders, [app])
        const target = next.find(folder => folder.id === id)
        if (target)
            target.apps.push(app)
        else
            next.push({ id: id, name: (folderById(id) || { name: i18n("Folder") }).name, apps: [app] })
        folders = next
        run(portalBin + " --folder-add " + shq(id) + " " + shq(app))
    }
    function removeFromFolder(app) {
        folders = withoutApps(folders, [app])
        run(portalBin + " --folder-remove " + shq(app))
    }
    function renameFolder(id, name) {
        const label = String(name || "").trim()
        if (label === "")
            return
        folders = folders.map(folder => folder.id === id ? { id: folder.id, name: label, apps: folder.apps } : folder)
        run(portalBin + " --folder-rename " + shq(id) + " " + shq(label))
    }
    function deleteFolder(id) {
        folders = folders.filter(folder => folder.id !== id)
        run(portalBin + " --folder-delete " + shq(id))
    }
    function trackApp(favoriteId) {
        const key = desktopKey(favoriteId)
        if (key !== "")
            run(portalBin + " --track-app " + shq(key))
    }

    property var learned: {
        try {
            return JSON.parse(launcherData.config.learnedRanking || "{}")
        } catch (error) {
            return {}
        }
    }
    function learn(query, key) {
        const term = String(query || "").trim().toLowerCase()
        if (term === "" || !key)
            return
        const next = Object.assign({}, learned)
        for (let length = 1; length <= Math.min(term.length, 24); ++length) {
            const prefix = term.substring(0, length)
            const counts = Object.assign({}, next[prefix] || {})
            counts[key] = (counts[key] || 0) + 1
            next[prefix] = counts
        }
        learned = next
        launcherData.config.learnedRanking = JSON.stringify(next)
    }
    function learnedFor(query) {
        const term = String(query || "").trim().toLowerCase()
        const counts = learned[term.substring(0, 24)]
        if (!counts)
            return []
        return Object.keys(counts).sort((a, b) => counts[b] - counts[a])
    }

    function relativeTime(seconds) {
        if (!seconds)
            return ""
        const minutes = Math.max(0, (Date.now() / 1000 - seconds) / 60)
        if (minutes < 60)
            return i18n("just now")
        if (minutes < 1440)
            return i18np("%1 hour ago", "%1 hours ago", Math.round(minutes / 60))
        const days = Math.round(minutes / 1440)
        if (days === 1)
            return i18n("yesterday")
        if (days < 14)
            return i18np("%1 day ago", "%1 days ago", days)
        if (days < 60)
            return i18np("%1 week ago", "%1 weeks ago", Math.round(days / 7))
        return Qt.formatDate(new Date(seconds * 1000), Qt.locale().dateFormat(Locale.ShortFormat))
    }
    function friendsFor(game) {
        return game && game.appid && friendsByAppid[game.appid] ? friendsByAppid[game.appid] : []
    }

    property string friendsPath: ""
    P5Support.DataSource {
        id: steamKeyWriter
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            launcherData.steamKeyBusy = false
            const failed = Number(result["exit code"] || 0) !== 0
            launcherData.steamKeyError = failed
            launcherData.steamKeyResult = String(failed ? (result.stderr || result.stdout) : i18n("Steam API key saved")).trim()
            launcherData.readFriends()
        }
    }
    function setSteamApiKey(key) {
        const clean = String(key).trim()
        if (clean === "")
            return
        steamKeyBusy = true
        steamKeyResult = ""
        steamKeyError = false
        steamKeyWriter.connectSource("$HOME/.local/bin/portal-friends --set-key " + shq(clean) + " # " + Date.now())
    }
    P5Support.DataSource {
        id: pathSource
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            launcherData.friendsPath = (result.stdout || "").trim()
            launcherData.readFriends()
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
            launcherData.friendsError = parsed.error || ""
            launcherData.friendsNeedsApiKey = parsed.needs_api_key === true || launcherData.friendsError.indexOf("steam_api_key") >= 0
            const list = (parsed.friends || []).slice().sort((a, b) => {
                const rank = f => f.ingame ? 0 : f.state > 0 ? 1 : 2
                const diff = rank(a) - rank(b)
                return diff !== 0 ? diff : String(a.name).toLowerCase().localeCompare(String(b.name).toLowerCase())
            })
            launcherData.friends = list
            launcherData.friendsByAppid = parsed.by_appid || {}
            const byGame = {}
            for (const friend of list) {
                if (!friend.ingame)
                    continue
                const key = friend.appid || friend.game
                if (!byGame[key]) {
                    const owned = launcherData.games.find(game => friend.appid && game.appid === friend.appid) || null
                    byGame[key] = { key: key, appid: friend.appid || "", name: friend.game, header: friend.header || "", game: owned, friends: [] }
                }
                byGame[key].friends.push(friend)
            }
            launcherData.playingNow = Object.values(byGame).sort((a, b) => b.friends.length - a.friends.length || String(a.name).localeCompare(String(b.name)))
            launcherData.friendsOnline = list.filter(f => f.state > 0).length
            launcherData.friendsInGame = list.filter(f => f.ingame).length
        }
        xhr.send()
    }
    FileWatcher {
        path: launcherData.live && launcherData.friendsEnabled ? launcherData.friendsPath : ""
        onChanged: launcherData.readFriends()
    }

    onLiveChanged: {
        if (!live)
            return
        readSidebar()
        refreshGames()
        if (friendsPath === "")
            pathSource.connectSource("printf %s \"$XDG_RUNTIME_DIR/Plasma-App-Portal/friends.json\"")
        else
            readFriends()
    }
}
