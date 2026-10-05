import QtQuick
import org.kde.plasma.plasma5support as P5Support
import "lib"

LauncherPinsData {
    id: launcherData

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
        return key.split(/\+(?!$)/).map(part => names[part.toLowerCase()] || part).join(" + ")
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

    Component.onCompleted: {
        readHidden()
        readFolders()
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
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            if (!xhr.responseText) {
                launcherData.friendsError = i18n("No snapshot — is the collector running?")
                launcherData.friendsNeedsApiKey = false
                return
            }
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
