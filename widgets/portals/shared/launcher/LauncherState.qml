import QtQuick
import org.kde.kirigami as Kirigami

FocusScope {
    id: launcher

    property bool compact: false
    signal closeFinished()
    signal activateRequested()
    readonly property bool menuOpen: menu.visible
    property bool shown: false
    property real progress: 0
    property real contentProgress: 0
    property bool hadFocus: false
    property string page: "home"
    property var visited: ({ home: true })
    property int sectionIndex: 0
    property int railIndex: -1
    property var sidebarDrag: null
    property Item hoveredPin: null
    property bool touchMode: false
    property bool touchDown: false
    property var pendingMenu: null
    property bool activationPending: false
    property point restingPointer
    property bool pointerKnown: false
    readonly property point pointerAt: pointerWatch.point.scenePosition
    onPointerAtChanged: {
        restingPointer = pointerAt
        pointerKnown = true
    }
    onShownChanged: pointerKnown = false
    HoverHandler {
        id: pointerWatch
    }
    function pointerMoved(at) {
        return pointerKnown && (Math.abs(at.x - restingPointer.x) >= 0.5 || Math.abs(at.y - restingPointer.y) >= 0.5)
    }
    readonly property real railPinHeight: Kirigami.Units.gridUnit * (compact ? 2.3 : 2.5)
    readonly property real railPinIcon: compact ? Kirigami.Units.iconSizes.smallMedium + 4 : Kirigami.Units.iconSizes.medium

    readonly property color ink: Kirigami.Theme.textColor
    readonly property color hoverFill: Qt.alpha(ink, 0.06)
    readonly property color selectedFill: Qt.alpha(ink, 0.13)
    readonly property color selectedLine: Qt.alpha(ink, 0.35)
    readonly property color hairline: Qt.alpha(ink, 0.09)
    readonly property color well: Qt.alpha(ink, 0.05)

    readonly property string rawQuery: field.text
    property string presentedQuery: ""
    readonly property bool searchSettled: presentedQuery === rawQuery
    onRawQueryChanged: {
        activationPending = false
        if (rawQuery.trim() === "") {
            searchSettle.stop()
            presentedQuery = rawQuery
        } else {
            searchSettle.restart()
        }
    }
    Timer {
        id: searchSettle
        interval: 60
        onTriggered: launcher.presentedQuery = launcher.rawQuery
    }
    function settleSearch() {
        searchSettle.stop()
        presentedQuery = rawQuery
    }
    function modeFor(text) {
        if (text.startsWith("g ")) return "games"
        if (text.startsWith("f ")) return "files"
        if (text.startsWith("a ")) return "apps"
        if (text.startsWith("s ")) return "packages"
        if (text.startsWith("@")) return "friends"
        if (text.startsWith("=")) return "calc"
        if (text.startsWith(">")) return "command"
        return "all"
    }
    function termFor(text) {
        const queryMode = modeFor(text)
        if (queryMode === "games" || queryMode === "files" || queryMode === "apps" || queryMode === "packages") return text.substring(2).trim()
        if (queryMode === "friends" || queryMode === "calc" || queryMode === "command") return text.substring(1).trim()
        return text.trim()
    }
    readonly property string mode: modeFor(rawQuery)
    readonly property string term: termFor(rawQuery)
    readonly property string presentedMode: modeFor(presentedQuery)
    readonly property string presentedTerm: termFor(presentedQuery)
    readonly property bool searching: rawQuery.trim() !== ""
    function currentView() {
        if (searching)
            return searchLoader.item
        const loader = pageLoaders.itemAt(pageIndex)
        return loader ? loader.item : null
    }

    readonly property var pageDefs: {
        const defs = [
            { key: "home", label: i18n("Home"), hint: i18n("Pins, friends and recent"), icon: "go-home-symbolic" },
            { key: "apps", label: i18n("Apps"), hint: i18n("Every application"), icon: "view-app-grid-symbolic" }
        ]
        if (launcherData.config.showGames)
            defs.push({ key: "games", label: i18n("Games"), hint: i18n("Your library"), icon: "input-gamepad-symbolic" })
        defs.push({ key: "files", label: i18n("Files"), hint: i18n("Places and recent documents"), icon: "folder-documents-symbolic" })
        if (launcherData.config.showFriends)
            defs.push({ key: "friends", label: i18n("Friends"), hint: i18n("Who is online and playing"), icon: "system-users-symbolic" })
        defs.push({ key: "system", label: i18n("System"), hint: i18n("Session and settings"), icon: "system-shutdown-symbolic" })
        defs.push({ key: "shortcuts", label: i18n("Shortcuts"), hint: i18n("Every keyboard shortcut, shown"), icon: "input-keyboard-symbolic" })
        defs.push({ key: "settings", label: i18n("Settings"), hint: i18n("Konveyor settings"), icon: "configure-symbolic" })
        return defs
    }
    readonly property var sidebarKeys: ({ friends: "showSidebarFriends", system: "showSidebarSystem", settings: "showSidebarSettings" })
    function onSidebar(key) {
        return !(key in sidebarKeys) || launcherData.config[sidebarKeys[key]]
    }
    readonly property int pageIndex: Math.max(0, pageDefs.findIndex(def => def.key === page))
    property bool altHeld: false
    property string openFolder: ""
    function toggleFolder(id) {
        openFolder = openFolder === id ? "" : id
        Qt.callLater(resetSelection)
    }
    function pinDrop(entries, from, to, into) {
        const source = entries[from]
        const target = entries[to]
        if (!source || !target || from === to)
            return
        if (into && source.kind === "app") {
            if (target.kind === "folder") {
                launcherData.addToFolder(target.id, source.favoriteId)
            } else {
                openFolder = launcherData.createFolder([target.favoriteId, source.favoriteId])
            }
            return
        }
        const favorites = launcherData.favorites
        if (source.kind === "app") {
            favorites.moveRow(source.favIndex, target.favIndex)
            return
        }
        const members = source.apps.map(index => launcherData.favoriteIds[index])
        for (let k = 0; k < members.length; ++k) {
            launcherData.rebuildPinned()
            const ids = launcherData.favoriteIds
            const current = ids.indexOf(members[k])
            if (current < 0)
                continue
            let destination = target.favIndex
            if (k > 0) {
                const previous = ids.indexOf(members[k - 1])
                destination = current < previous ? previous : previous + 1
            }
            if (destination !== current)
                favorites.moveRow(current, destination)
        }
        launcherData.rebuildPinned()
    }
    function folderEntries(folder) {
        const entries = [{ text: launcher.openFolder === folder.id ? i18n("Close folder") : i18n("Open folder"), icon: "folder-open-symbolic", run: () => launcher.toggleFolder(folder.id) }]
        entries.push({ text: i18n("Rename…"), icon: "edit-rename", run: () => { launcher.openFolder = folder.id; launcher.renameRequested(folder.id) } })
        entries.push({ separator: true })
        entries.push({ text: i18n("Ungroup"), icon: "edit-delete-remove", run: () => { if (launcher.openFolder === folder.id) launcher.openFolder = ""; launcherData.deleteFolder(folder.id) } })
        return entries
    }
    signal renameRequested(string id)

    function markVisited(key) {
        if (visited[key] !== true) {
            const next = Object.assign({}, visited)
            next[key] = true
            visited = next
        }
    }
    function focusSearch() {
        field.forceActiveFocus()
    }
    function setQuery(text) {
        field.text = text
        field.cursorPosition = text.length
        field.forceActiveFocus()
    }
    function shownPage(key) {
        return pageDefs.some(def => def.key === key) ? key : "home"
    }
    function goToPage(key) {
        openFolder = ""
        railIndex = -1
        page = shownPage(key)
        markVisited(page)
        field.text = ""
        field.forceActiveFocus()
        Qt.callLater(resetSelection)
    }
    function stepPage(delta) {
        const next = (pageIndex + delta + pageDefs.length) % pageDefs.length
        goToPage(pageDefs[next].key)
    }
}
