import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import "lib"

PlasmoidItem {
    id: root

    readonly property string panelIcon: Plasmoid.configuration.icon || "im-user"
    Plasmoid.icon: panelIcon
    Plasmoid.title: i18n("Steam Friends")

    property var friends: []
    property var friendsById: ({})
    property string error: ""
    property bool saving: false
    property bool ready: false
    property string searchText: ""
    property string tabKey: Plasmoid.configuration.currentTab || "all"
    readonly property string sortMode: Plasmoid.configuration.sortMode
    readonly property bool hideOffline: Plasmoid.configuration.hideOffline
    readonly property string favorites: Plasmoid.configuration.favorites
    property var sectionCounts: ({})

    onSearchTextChanged: rebuild()
    onSortModeChanged: rebuild()
    onHideOfflineChanged: rebuild()
    onFavoritesChanged: rebuild()
    onTabKeyChanged: {
        Plasmoid.configuration.currentTab = tabKey
        rebuild()
    }

    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical
    property bool popupAlive: !inPanel
    preferredRepresentation: inPanel ? compactRepresentation : fullRepresentation
    onExpandedChanged: function() {
        if (root.expanded) {
            releasePopup.stop()
            popupAlive = true
            read()
        } else if (inPanel) {
            releasePopup.restart()
        }
    }
    Timer {
        id: releasePopup
        interval: 1500
        onTriggered: root.popupAlive = root.expanded || !root.inPanel
    }

    function shq(s) { return "'" + String(s).replace(/'/g, "'\\''") + "'" }

    function favoritesList() {
        return (root.favorites || "").split(",").filter(s => s !== "")
    }
    function isFavorite(sid) { return favoritesList().indexOf(String(sid)) >= 0 }
    function toggleFavorite(sid) {
        sid = String(sid)
        const list = favoritesList()
        const at = list.indexOf(sid)
        if (at >= 0)
            list.splice(at, 1)
        else
            list.push(sid)
        Plasmoid.configuration.favorites = list.join(",")
    }

    readonly property color cInGame: "#90ba3c"
    readonly property color cOnline: "#57cbde"
    readonly property color cAway: "#7e9bb5"
    readonly property color cOffline: "#6a6a6a"
    function isOnline(f) { return !!f && (f.ingame || f.state !== 0) }
    function stateColor(f) {
        if (!f) return cOffline
        if (f.ingame) return cInGame
        if (f.state === 0) return cOffline
        if (f.state === 2 || f.state === 3 || f.state === 4) return cAway
        return cOnline
    }
    function stateText(f) {
        if (!f) return ""
        if (f.ingame) return f.game || i18n("In game")
        switch (f.state) {
        case 0: return i18n("Offline")
        case 2: return i18n("Busy")
        case 3: return i18n("Away")
        case 4: return i18n("Snooze")
        default: return i18n("Online")
        }
    }

    readonly property int onlineCount: friends.filter(f => isOnline(f)).length
    readonly property int inGameCount: friends.filter(f => f.ingame).length
    readonly property int favoriteCount: friends.filter(f => favoritesList().indexOf(String(f.steamid)) >= 0).length

    readonly property var playingNow: {
        const groups = {}
        const order = []
        for (const f of friends) {
            if (!f.ingame)
                continue
            const key = f.appid || f.game || "?"
            if (!groups[key]) {
                groups[key] = { key: key, appid: f.appid || "", game: f.game || i18n("In game"), capsule: f.capsule || "", friends: [] }
                order.push(key)
            }
            groups[key].friends.push(f)
        }
        return order.map(key => groups[key]).sort((a, b) => b.friends.length - a.friends.length || a.game.localeCompare(b.game))
    }

    function lastOnlineText(f) {
        if (!f || f.ingame || f.state !== 0) return ""
        const t = f.lastlogoff || 0
        if (!t) return ""
        const diff = Date.now() / 1000 - t
        if (diff < 60) return i18n("just now")
        const y = Math.floor(diff / 31536000)
        if (y >= 1) return i18n("%1y ago", y)
        const d = Math.floor(diff / 86400)
        if (d >= 1) return i18n("%1d ago", d)
        const h = Math.floor(diff / 3600)
        if (h >= 1) return i18n("%1h ago", h)
        return i18n("%1m ago", Math.floor(diff / 60))
    }

    function flagEmoji(cc) {
        if (!cc || String(cc).length !== 2) return ""
        const s = String(cc).toUpperCase()
        const a = s.charCodeAt(0) - 65, b = s.charCodeAt(1) - 65
        if (a < 0 || a > 25 || b < 0 || b > 25) return ""
        return String.fromCodePoint(0x1F1E6 + a) + String.fromCodePoint(0x1F1E6 + b)
    }

    function sectionOf(f, fav) {
        if (fav) return i18n("Favourites")
        if (f.ingame) return i18n("In Game")
        if (f.state === 0) return i18n("Offline")
        return i18n("Online")
    }
    function sectionRank(f, fav) {
        if (fav) return 0
        if (f.ingame) return 1
        if (f.state === 0) return 3
        return 2
    }

    property string cachePath: ""
    P5Support.DataSource {
        id: pathHelper
        engine: "executable"
        onNewData: function(source, d) {
            root.cachePath = (d.stdout || "").trim()
            disconnectSource(source)
            root.read()
        }
    }
    function read() {
        if (!cachePath) return
        const xhr = new XMLHttpRequest()
        xhr.open("GET", "file://" + cachePath)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE) return
            if (!xhr.responseText) {
                root.error = i18n("No snapshot — is the collector running?")
                return
            }
            root.processSnapshot(xhr.responseText)
        }
        xhr.send()
    }
    function processSnapshot(text) {
        let s
        try { s = JSON.parse(text || "{}") } catch (e) { root.error = i18n("Could not read friends data"); return }
        root.error = (s.ok === false) ? (s.error || i18n("No data")) : ""
        root.saving = false
        const list = s.friends || []
        const byId = {}
        for (const f of list)
            byId[String(f.steamid)] = f
        root.friends = list
        root.friendsById = byId
        root.ready = true
        root.rebuild()
    }

    function matches(f, q) {
        return q === "" || (f.name || "").toLowerCase().indexOf(q) >= 0 || (f.game || "").toLowerCase().indexOf(q) >= 0
    }

    function rebuild() {
        const q = root.searchText.trim().toLowerCase()
        const favs = root.favoritesList()
        const dir = root.sortMode === "name_desc" ? -1 : 1
        const rows = []
        const counts = {}
        for (const f of root.friends) {
            const fav = favs.indexOf(String(f.steamid)) >= 0
            if (q === "") {
                if (root.tabKey === "ingame" && !f.ingame) continue
                if (root.tabKey === "online" && !root.isOnline(f)) continue
                if (root.tabKey === "favorites" && !fav) continue
                if (root.hideOffline && !root.isOnline(f)) continue
            } else if (!root.matches(f, q)) {
                continue
            }
            const section = root.tabKey === "favorites" && q === "" ? root.sectionOf(f, false) : root.sectionOf(f, fav)
            counts[section] = (counts[section] || 0) + 1
            rows.push({ steamid: String(f.steamid), name: f.name || "", fav: fav, section: section,
                        rank: root.tabKey === "favorites" && q === "" ? root.sectionRank(f, false) : root.sectionRank(f, fav) })
        }
        rows.sort((x, y) => x.rank !== y.rank ? x.rank - y.rank : dir * x.name.localeCompare(y.name))
        root.sectionCounts = counts
        root.syncModel(rows.map(r => ({ steamid: r.steamid, section: r.section, fav: r.fav })))
    }

    ListModel { id: rowModel }
    readonly property alias rows: rowModel
    function syncModel(desired) {
        const want = {}
        for (const d of desired)
            want[d.steamid] = true
        for (let r = rowModel.count - 1; r >= 0; r--)
            if (want[rowModel.get(r).steamid] !== true)
                rowModel.remove(r)
        for (let pos = 0; pos < desired.length; pos++) {
            const d = desired[pos]
            if (pos < rowModel.count && rowModel.get(pos).steamid === d.steamid) {
                const a = rowModel.get(pos)
                if (a.section !== d.section || a.fav !== d.fav)
                    rowModel.set(pos, d)
                continue
            }
            let cur = -1
            for (let x = pos + 1; x < rowModel.count; x++) {
                if (rowModel.get(x).steamid === d.steamid) {
                    cur = x
                    break
                }
            }
            if (cur < 0) {
                rowModel.insert(pos, d)
            } else {
                rowModel.move(cur, pos, 1)
                const b = rowModel.get(pos)
                if (b.section !== d.section || b.fav !== d.fav)
                    rowModel.set(pos, d)
            }
        }
    }

    P5Support.DataSource {
        id: runner
        engine: "executable"
        onNewData: function(source, d) { disconnectSource(source) }
    }
    function steamRun(url) {
        if (url)
            runner.connectSource("steam " + root.shq(url))
    }
    function openChat(f) {
        if (f && f.chat) {
            steamRun(f.chat)
            root.expanded = false
        }
    }
    function launch(command) { runner.connectSource(command) }

    function saveKey(k) {
        k = String(k).trim()
        if (k === "") return
        root.saving = true
        runner.connectSource("$HOME/.local/bin/portal-friends --set-key " + root.shq(k)
            + " ; systemctl --user restart portal-friends.service")
        setupReloadTimer.restart()
    }
    Timer { id: setupReloadTimer; interval: 4000; onTriggered: root.read() }

    FileWatcher { path: root.cachePath; onChanged: root.read() }
    Component.onCompleted: pathHelper.connectSource("printf %s \"$XDG_RUNTIME_DIR/Plasma-App-Portal/friends.json\"")

    signal menuRequested(var friend)

    toolTipMainText: i18n("Steam Friends")
    toolTipSubText: {
        if (error !== "")
            return error
        const lines = [i18n("%1 online · %2 in game · %3 friends", onlineCount, inGameCount, friends.length)]
        for (const group of playingNow.slice(0, 5))
            lines.push(i18n("%1: %2", group.game, group.friends.map(f => f.name).join(", ")))
        return lines.join("\n")
    }

    compactRepresentation: CompactView {}
    fullRepresentation: FullView {}
}
