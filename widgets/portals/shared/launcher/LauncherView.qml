import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents

FocusScope {
    id: launcher

    property bool compact: false
    readonly property bool wanted: root.open
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
    property bool warm: false
    Timer {
        id: warmTimer
        interval: Kirigami.Units.longDuration * 2
        onTriggered: launcher.warm = true
    }

    readonly property color ink: Kirigami.Theme.textColor
    readonly property color hoverFill: Qt.alpha(ink, 0.06)
    readonly property color selectedFill: Qt.alpha(ink, 0.13)
    readonly property color selectedLine: Qt.alpha(ink, 0.35)
    readonly property color hairline: Qt.alpha(ink, 0.09)
    readonly property color well: Qt.alpha(ink, 0.05)


    readonly property string rawQuery: field.text
    readonly property string mode: {
        const text = rawQuery
        if (text.startsWith("g ")) return "games"
        if (text.startsWith("f ")) return "files"
        if (text.startsWith("a ")) return "apps"
        if (text.startsWith("s ")) return "packages"
        if (text.startsWith("@")) return "friends"
        if (text.startsWith("=")) return "calc"
        if (text.startsWith(">")) return "command"
        return "all"
    }
    readonly property string term: {
        const text = rawQuery
        if (mode === "games" || mode === "files" || mode === "apps" || mode === "packages") return text.substring(2).trim()
        if (mode === "friends" || mode === "calc" || mode === "command") return text.substring(1).trim()
        return text.trim()
    }
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
        if (Plasmoid.configuration.showGames)
            defs.push({ key: "games", label: i18n("Games"), hint: i18n("Your library"), icon: "input-gamepad-symbolic" })
        defs.push({ key: "files", label: i18n("Files"), hint: i18n("Places and recent documents"), icon: "folder-documents-symbolic" })
        if (Plasmoid.configuration.showFriends)
            defs.push({ key: "friends", label: i18n("Friends"), hint: i18n("Who is online and playing"), icon: "system-users-symbolic" })
        defs.push({ key: "system", label: i18n("System"), hint: i18n("Session and settings"), icon: "system-shutdown-symbolic" })
        defs.push({ key: "shortcuts", label: i18n("Shortcuts"), hint: i18n("Every keyboard shortcut, shown"), icon: "input-keyboard-symbolic" })
        defs.push({ key: "settings", label: i18n("Settings"), hint: i18n("Konveyor settings"), icon: "configure-symbolic" })
        return defs
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

    LauncherData {
        id: launcherData
        applet: root
        live: launcher.shown
        query: launcher.term
        searchMode: launcher.mode
    }

    function applyPendingPins() {
        if (root.pendingPins.length === 0)
            return
        for (const path of root.pendingPins) {
            if (!launcherData.favorites.isFavorite(path))
                launcherData.favorites.addFavorite(path)
        }
        root.pendingPins = []
    }
    Connections {
        target: root
        function onPinsRequested() { launcher.applyPendingPins() }
    }
    onWantedChanged: wanted ? openNow() : closeNow()
    Timer {
        interval: 0
        running: true
        onTriggered: {
            launcher.applyPendingPins()
            if (launcher.wanted && !launcher.shown)
                launcher.openNow()
        }
    }

    function openNow() {
        closeAnimation.stop()
        const wantedPage = root.requestedPage || Plasmoid.configuration.defaultPage
        root.requestedPage = ""
        page = pageDefs.some(def => def.key === wantedPage) ? wantedPage : "home"
        markVisited(page)
        openFolder = ""
        field.text = ""
        hadFocus = false
        shown = true
        activateRequested()
        field.forceActiveFocus()
        openAnimation.restart()
        warmTimer.restart()
        Qt.callLater(resetSelection)
    }
    function closeNow() {
        openAnimation.stop()
        menu.close()
        closeAnimation.restart()
    }

    ParallelAnimation {
        id: openAnimation
        NumberAnimation { target: launcher; property: "progress"; to: 1; duration: Kirigami.Units.longDuration * 1.4; easing.type: Easing.OutCubic }
        SequentialAnimation {
            PauseAnimation { duration: Kirigami.Units.shortDuration * 0.5 }
            NumberAnimation { target: launcher; property: "contentProgress"; to: 1; duration: Kirigami.Units.longDuration * 1.3; easing.type: Easing.OutCubic }
        }
    }
    SequentialAnimation {
        id: closeAnimation
        ParallelAnimation {
            NumberAnimation { target: launcher; property: "progress"; to: 0; duration: Kirigami.Units.longDuration * 0.8; easing.type: Easing.InCubic }
            NumberAnimation { target: launcher; property: "contentProgress"; to: 0; duration: Kirigami.Units.shortDuration; easing.type: Easing.InCubic }
        }
        ScriptAction {
            script: {
                launcher.shown = false
                field.text = ""
                launcher.closeFinished()
            }
        }
    }

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
    function goToPage(key) {
        if (key === "settings") {
            launcherData.openKonveyorSettings()
            root.hide()
            return
        }
        openFolder = ""
        page = key
        markVisited(key)
        field.text = ""
        field.forceActiveFocus()
        Qt.callLater(resetSelection)
    }
    function stepPage(delta) {
        const next = (pageIndex + delta + pageDefs.length) % pageDefs.length
        goToPage(pageDefs[next].key)
    }

    function liveSections() {
        const view = currentView()
        if (!view || !view.sections)
            return []
        return view.sections.filter(section => section && section.visible && section.shownCount > 0)
    }
    function applySection(sections, index, scroll) {
        const view = currentView()
        const all = view && view.sections ? view.sections : []
        for (const section of all) {
            if (section)
                section.sectionActive = false
        }
        sectionIndex = index
        const current = sections[index]
        if (!current)
            return
        current.sectionActive = true
        if (scroll !== false && view.column && current.currentIndex >= 0 && !current.scrolling)
            ensureVisible(view, current)
    }
    function ensureVisible(view, section) {
        const item = section.itemAtIndex(section.currentIndex)
        const flick = view.contentItem
        if (!item || !flick)
            return
        const top = item.mapToItem(view.column, 0, 0).y
        const headerRoom = section.currentIndex < section.columns ? Kirigami.Units.gridUnit * 2.4 : Kirigami.Units.largeSpacing
        const wantTop = top - headerRoom
        const wantBottom = top + item.height + Kirigami.Units.largeSpacing
        let target = flick.contentY
        if (wantTop < flick.contentY)
            target = wantTop
        else if (wantBottom > flick.contentY + flick.height)
            target = wantBottom - flick.height
        target = Math.max(0, Math.min(target, view.column.height - flick.height))
        if (Math.abs(target - flick.contentY) < 1)
            return
        scrollAnimation.target = flick
        scrollAnimation.to = target
        scrollAnimation.restart()
    }
    NumberAnimation {
        id: scrollAnimation
        property: "contentY"
        duration: Kirigami.Units.longDuration
        easing.type: Easing.OutCubic
    }
    function resetSelection() {
        const sections = liveSections()
        if (sections.length === 0)
            return
        for (const section of sections)
            section.currentIndex = -1
        sections[0].reset()
        applySection(sections, 0)
    }
    function select(section, index) {
        if (section.currentIndex === index && section.sectionActive)
            return
        const sections = liveSections()
        const at = sections.indexOf(section)
        if (at < 0)
            return
        if (sections[sectionIndex] && sectionIndex !== at)
            sections[sectionIndex].currentIndex = -1
        section.currentIndex = index
        applySection(sections, at, false)
    }
    function ensureSelection() {
        const current = currentSection()
        if (!current || current.currentIndex < 0 || current.currentIndex >= current.shownCount)
            resetSelection()
    }
    function currentSection() {
        const sections = liveSections()
        if (sectionIndex >= sections.length)
            return null
        return sections[sectionIndex]
    }
    function navigate(dx, dy) {
        const sections = liveSections()
        if (sections.length === 0)
            return
        if (sectionIndex >= sections.length) {
            resetSelection()
            return
        }
        const current = sections[sectionIndex]
        if (current.currentIndex < 0) {
            current.reset()
            applySection(sections, sectionIndex)
            return
        }
        if (current.move(dx, dy)) {
            applySection(sections, sectionIndex)
            return
        }
        if (dy > 0 && sectionIndex + 1 < sections.length) {
            current.currentIndex = -1
            sections[sectionIndex + 1].enterFrom(false)
            applySection(sections, sectionIndex + 1)
        } else if (dy < 0 && sectionIndex > 0) {
            current.currentIndex = -1
            sections[sectionIndex - 1].enterFrom(true)
            applySection(sections, sectionIndex - 1)
        }
    }
    function stepSection(forward) {
        const sections = liveSections()
        if (sections.length <= 1) {
            const view = currentView()
            if (view && view.cycle)
                view.cycle(forward)
            return
        }
        const next = (sectionIndex + (forward ? 1 : -1) + sections.length) % sections.length
        if (sections[sectionIndex])
            sections[sectionIndex].currentIndex = -1
        sections[next].reset()
        applySection(sections, next)
    }
    function activateCurrent() {
        const section = currentSection()
        if (section)
            section.activate()
    }
    function menuForCurrent() {
        const section = currentSection()
        if (section)
            section.openMenu()
    }
    function pinCurrent() {
        const section = currentSection()
        if (!section || section.currentIndex < 0)
            return
        const item = section.itemAtIndex(section.currentIndex)
        if (item && item.favoriteId)
            togglePin(item.favoriteId)
    }

    function isPinned(favoriteId) {
        return favoriteId !== "" && launcherData.favorites.isFavorite(favoriteId)
    }
    function togglePin(favoriteId) {
        if (!favoriteId)
            return
        if (launcherData.favorites.isFavorite(favoriteId))
            launcherData.favorites.removeFavorite(favoriteId)
        else
            launcherData.favorites.addFavorite(favoriteId)
    }
    function trigger(model, index, key) {
        remember(key)
        if (key && String(key).indexOf(".desktop") >= 0)
            launcherData.trackApp(key)
        if (model && model.trigger(index, "", null) !== false)
            root.hide()
    }
    function kickerEntries(model, index, actions, favoriteId) {
        const entries = [{ text: i18n("Open"), icon: "system-run", run: () => launcher.trigger(model, index, favoriteId) }]
        if (favoriteId) {
            const pinned = isPinned(favoriteId)
            entries.push({ text: pinned ? i18n("Unpin from Home") : i18n("Pin to Home"), icon: pinned ? "window-unpin" : "window-pin", run: () => launcher.togglePin(favoriteId) })
            if (pinned) {
                const inside = launcherData.folderFor(favoriteId)
                if (inside)
                    entries.push({ text: i18n("Remove from “%1”", inside.name), icon: "folder-remove", run: () => launcherData.removeFromFolder(favoriteId) })
                for (const folder of launcherData.folders.filter(folder => !inside || folder.id !== inside.id).slice(0, 6))
                    entries.push({ text: i18n("Move to “%1”", folder.name), icon: "folder-symbolic", run: () => launcherData.addToFolder(folder.id, favoriteId) })
            }
            if (favoriteId.indexOf(".desktop") >= 0)
                entries.push({ text: i18n("Hide from launcher"), icon: "view-hidden", run: () => launcherData.setHidden(favoriteId, true) })
        }
        const list = actions || []
        if (list.length > 0)
            entries.push({ separator: true })
        for (const action of list) {
            if (!action || action.type === "separator" || action.text === undefined) {
                if (entries.length > 0 && !entries[entries.length - 1].separator)
                    entries.push({ separator: true })
                continue
            }
            entries.push({
                text: action.text,
                icon: action.icon || "",
                run: () => {
                    if (model.trigger(index, action.actionId, action.actionArgument) !== false)
                        root.hide()
                }
            })
        }
        if (entries.length > 0 && entries[entries.length - 1].separator)
            entries.pop()
        return entries
    }
    function gameEntries(game) {
        const entries = [{ text: i18n("Play"), icon: "media-playback-start", run: () => { launcher.remember("game:" + game.id); launcherData.launchGame(game); root.hide() } }]
        if (game.appid) {
            entries.push({ separator: true })
            entries.push({ text: i18n("Store page"), icon: "internet-web-browser", run: () => { Qt.openUrlExternally("steam://store/" + game.appid); root.hide() } })
            entries.push({ text: i18n("Properties"), icon: "configure", run: () => { Qt.openUrlExternally("steam://gameproperties/" + game.appid); root.hide() } })
            entries.push({ text: i18n("Browse local files"), icon: "folder-open", run: () => { Qt.openUrlExternally("steam://open/games/details/" + game.appid); root.hide() } })
        }
        entries.push({ separator: true })
        entries.push({ text: i18n("Set custom art…"), icon: "insert-image", run: () => launcherData.pickArt(game) })
        if (game.custom_art)
            entries.push({ text: i18n("Reset art"), icon: "edit-undo", run: () => launcherData.resetArt(game) })
        for (const friend of launcherData.friendsFor(game)) {
            if (!entries[entries.length - 1].friendHeader && !entries.some(entry => entry.friendHeader)) {
                entries.push({ separator: true })
                entries.push({ text: i18n("Playing now"), friendHeader: true, disabled: true })
            }
            entries.push({ text: friend.name, icon: "im-user", iconSource: friend.avatar || "", run: () => { Qt.openUrlExternally(friend.chat); root.hide() } })
        }
        return entries
    }
    function friendEntries(friend) {
        const entries = [{ text: i18n("Open chat"), icon: "dialog-messages", run: () => { Qt.openUrlExternally(friend.chat); root.hide() } }]
        if (friend.join)
            entries.push({ text: i18n("Join game"), icon: "media-playback-start", run: () => { Qt.openUrlExternally(friend.join); root.hide() } })
        if (friend.ingame && friend.watch)
            entries.push({ text: i18n("Watch game"), icon: "view-visible", run: () => { Qt.openUrlExternally(friend.watch); root.hide() } })
        entries.push({ separator: true })
        entries.push({ text: i18n("View profile"), icon: "user-identity", run: () => { Qt.openUrlExternally(friend.profile); root.hide() } })
        if (friend.profile_web)
            entries.push({ text: i18n("Open profile in browser"), icon: "internet-web-browser", run: () => { Qt.openUrlExternally(friend.profile_web); root.hide() } })
        return entries
    }
    function sessionIcon(label) {
        const text = String(label).toLowerCase()
        if (text.indexOf("lock") >= 0) return "system-lock-screen-symbolic"
        if (text.indexOf("switch") >= 0) return "system-switch-user-symbolic"
        if (text.indexOf("log") >= 0) return "system-log-out-symbolic"
        if (text.indexOf("hibernate") >= 0) return "system-suspend-hibernate-symbolic"
        if (text.indexOf("sleep") >= 0 || text.indexOf("suspend") >= 0) return "system-suspend-symbolic"
        if (text.indexOf("restart") >= 0 || text.indexOf("reboot") >= 0) return "system-reboot-symbolic"
        if (text.indexOf("shut") >= 0 || text.indexOf("power off") >= 0) return "system-shutdown-symbolic"
        return "system-run-symbolic"
    }
    function powerEntries() {
        const model = launcherData.system
        const entries = []
        for (let row = 0; row < model.count; ++row) {
            const label = model.labelForRow(row)
            const at = row
            entries.push({ text: label, icon: sessionIcon(label), run: () => launcher.trigger(model, at) })
        }
        entries.push({ separator: true })
        entries.push({ text: i18n("Session page"), icon: "go-next-symbolic", run: () => launcher.goToPage("system") })
        return entries
    }
    function friendsQuickEntries(anchor) {
        const entries = []
        const playing = launcherData.friends.filter(friend => friend.ingame)
        const online = launcherData.friends.filter(friend => !friend.ingame && friend.state > 0)
        if (playing.length > 0)
            entries.push({ text: i18n("In game"), disabled: true })
        for (const friend of playing.slice(0, 12))
            entries.push({ text: i18n("%1 · %2", friend.name, friend.game), icon: "input-gamepad-symbolic", run: () => Qt.callLater(() => launcher.openMenu(launcher.friendEntries(friend), anchor)) })
        if (online.length > 0) {
            if (entries.length > 0)
                entries.push({ separator: true })
            entries.push({ text: i18n("Online"), disabled: true })
        }
        for (const friend of online.slice(0, 8))
            entries.push({ text: friend.name, icon: "user-available-symbolic", run: () => Qt.callLater(() => launcher.openMenu(launcher.friendEntries(friend), anchor)) })
        if (entries.length > 0)
            entries.push({ separator: true })
        entries.push({ text: i18n("All friends"), icon: "system-users-symbolic", run: () => launcher.goToPage("friends") })
        return entries
    }
    function remember(key) {
        if (searching && key)
            launcherData.learn(term, key)
    }
    function packageEntries(pkg) {
        return [
            { text: i18n("Install with Shelly"), icon: "shelly", run: () => { launcherData.installPackage(pkg); root.hide() } },
            { separator: true },
            { text: pkg.source === "aur" ? i18n("Open AUR page") : i18n("Open package page"), icon: "internet-web-browser", run: () => { Qt.openUrlExternally(pkg.page); root.hide() } },
            { text: i18n("Open project website"), icon: "globe", disabled: !pkg.url, run: () => { Qt.openUrlExternally(pkg.url); root.hide() } },
            { text: i18n("Copy name"), icon: "edit-copy", run: () => launcherData.copyText(pkg.name) }
        ]
    }
    function openMenu(entries, item) {
        menu.entries = entries
        if (item)
            menu.popup(item, item.width / 2, item.height / 2)
        else
            menu.popup()
    }

    FocusScope {
        id: content

        anchors.fill: parent
        opacity: launcher.progress
        focus: true

        Keys.forwardTo: [field]

        Rectangle {
            anchors.fill: parent
            radius: Kirigami.Units.cornerRadius * 2
            color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.14)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: launcher.compact ? Kirigami.Units.smallSpacing * 1.5 : Kirigami.Units.largeSpacing
            spacing: launcher.compact ? Kirigami.Units.largeSpacing : Kirigami.Units.largeSpacing * 1.5
            scale: 0.97 + 0.03 * launcher.progress
            transformOrigin: Item.Top

            Item {
                id: topBar
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * (launcher.compact ? 2.2 : 2.6)
                readonly property real gap: Kirigami.Units.largeSpacing * 2
                readonly property real sideWidth: Math.max(Kirigami.Units.gridUnit * 13, statusRow.implicitWidth)

                RowLayout {
                    id: identity
                    visible: !launcher.compact
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: topBar.sideWidth
                    spacing: Kirigami.Units.largeSpacing
                    Components.Avatar {
                        Layout.preferredWidth: Kirigami.Units.iconSizes.medium + Kirigami.Units.smallSpacing
                        Layout.preferredHeight: Layout.preferredWidth
                        source: launcherData.user.faceIconUrl
                        name: launcherData.user.fullName || launcherData.user.loginName
                    }
                    ColumnLayout {
                        spacing: 0
                        Layout.fillWidth: true
                        PlasmaComponents.Label {
                            text: launcherData.user.fullName || launcherData.user.loginName
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        PlasmaComponents.Label {
                            text: launcherData.user.host
                            font: Kirigami.Theme.smallFont
                            opacity: 0.55
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                Rectangle {
                    id: searchBox
                    anchors.centerIn: launcher.compact ? undefined : parent
                    anchors.verticalCenter: launcher.compact ? parent.verticalCenter : undefined
                    x: 0
                    width: launcher.compact ? parent.width - statusRow.implicitWidth - Kirigami.Units.largeSpacing : Math.max(Kirigami.Units.gridUnit * 16, Math.min(Kirigami.Units.gridUnit * 44, parent.width - (topBar.sideWidth + topBar.gap) * 2))
                    height: parent.height
                    radius: height / 2
                    color: field.activeFocus ? Qt.alpha(launcher.ink, 0.09) : launcher.well
                    border.width: 1
                    border.color: field.activeFocus ? Qt.alpha(launcher.ink, 0.22) : launcher.hairline

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Kirigami.Units.largeSpacing * 1.5
                        anchors.rightMargin: Kirigami.Units.smallSpacing
                        spacing: Kirigami.Units.largeSpacing

                        Kirigami.Icon {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.small
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
                            source: "search-symbolic"
                            color: launcher.ink
                            isMask: true
                            opacity: 0.6
                        }
                        QQC2.TextField {
                            id: field
                            Layout.fillWidth: true
                            Layout.preferredWidth: 0
                            background: null
                            leftPadding: 0
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
                            placeholderText: i18n("Search apps, games, files, settings, friends and packages")
                            onTextChanged: Qt.callLater(launcher.resetSelection)
                            Keys.onPressed: function(event) {
                                const ctrl = event.modifiers & Qt.ControlModifier
                                const alt = event.modifiers & Qt.AltModifier
                                if (event.key === Qt.Key_Escape) {
                                    if (field.text !== "")
                                        field.text = ""
                                    else
                                        root.hide()
                                } else if (event.key === Qt.Key_Down) {
                                    launcher.navigate(0, 1)
                                } else if (event.key === Qt.Key_Up) {
                                    launcher.navigate(0, -1)
                                } else if (event.key === Qt.Key_Left && (field.text === "" || ctrl)) {
                                    launcher.navigate(-1, 0)
                                } else if (event.key === Qt.Key_Right && (field.text === "" || ctrl || field.cursorPosition === field.length)) {
                                    launcher.navigate(1, 0)
                                } else if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && alt) {
                                    launcher.menuForCurrent()
                                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                    launcher.activateCurrent()
                                } else if (event.key === Qt.Key_Menu) {
                                    launcher.menuForCurrent()
                                } else if (event.key === Qt.Key_Tab && ctrl) {
                                    launcher.stepPage(1)
                                } else if (event.key === Qt.Key_Backtab && ctrl) {
                                    launcher.stepPage(-1)
                                } else if (event.key === Qt.Key_Tab) {
                                    launcher.stepSection(true)
                                } else if (event.key === Qt.Key_Backtab) {
                                    launcher.stepSection(false)
                                } else if (ctrl && event.key === Qt.Key_Comma) {
                                    launcher.goToPage("settings")
                                } else if (ctrl && event.key === Qt.Key_P) {
                                    launcher.pinCurrent()
                                } else if (alt && event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
                                    const target = event.key - Qt.Key_1
                                    if (target < launcher.pageDefs.length)
                                        launcher.goToPage(launcher.pageDefs[target].key)
                                } else if (event.key === Qt.Key_Alt) {
                                    launcher.altHeld = true
                                    return
                                } else if (ctrl && event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
                                    const at = event.key - Qt.Key_1
                                    if (at < launcherData.favorites.count)
                                        launcher.trigger(launcherData.favorites, at)
                                } else {
                                    return
                                }
                                event.accepted = true
                            }
                            Keys.onReleased: function(event) {
                                if (event.key === Qt.Key_Alt)
                                    launcher.altHeld = false
                            }
                            onActiveFocusChanged: if (!activeFocus) launcher.altHeld = false
                        }
                        PlasmaComponents.Label {
                            visible: launcher.searching && searchLoader.item !== null && searchLoader.item.totalResults > 0
                            text: searchLoader.item ? i18np("%1 result", "%1 results", searchLoader.item.totalResults) : ""
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.45
                        }
                        Rectangle {
                            visible: launcher.searching && launcher.mode !== "all"
                            implicitWidth: modeLabel.implicitWidth + Kirigami.Units.largeSpacing * 1.5
                            implicitHeight: modeLabel.implicitHeight + Kirigami.Units.smallSpacing
                            radius: height / 2
                            color: Qt.alpha(launcher.ink, 0.12)
                            PlasmaComponents.Label {
                                id: modeLabel
                                anchors.centerIn: parent
                                text: ({ games: i18n("Games"), files: i18n("Files"), apps: i18n("Apps"), packages: i18n("Packages"), friends: i18n("Friends"), calc: i18n("Calculator"), command: i18n("Command") })[launcher.mode] || ""
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                font.weight: Font.DemiBold
                            }
                        }
                        PlasmaComponents.ToolButton {
                            visible: field.text !== ""
                            icon.name: "edit-clear-symbolic"
                            display: PlasmaComponents.AbstractButton.IconOnly
                            text: i18n("Clear")
                            onClicked: {
                                field.text = ""
                                field.forceActiveFocus()
                            }
                        }
                    }
                }

                RowLayout {
                    id: statusRow
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Kirigami.Units.smallSpacing

                    FriendsPill {
                        Layout.rightMargin: launcher.compact ? 0 : Kirigami.Units.largeSpacing
                    }

                    ColumnLayout {
                        visible: !launcher.compact
                        spacing: 0
                        Layout.rightMargin: Kirigami.Units.largeSpacing
                        PlasmaComponents.Label {
                            Layout.alignment: Qt.AlignRight
                            text: Qt.formatTime(launcherData.now, Qt.locale().timeFormat(Locale.ShortFormat))
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
                            font.weight: Font.DemiBold
                        }
                        PlasmaComponents.Label {
                            Layout.alignment: Qt.AlignRight
                            text: Qt.formatDate(launcherData.now, "ddd d MMM")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.55
                        }
                    }
                    PlasmaComponents.ToolButton {
                        icon.name: "configure-symbolic"
                        display: PlasmaComponents.AbstractButton.IconOnly
                        text: i18n("Settings (Ctrl+,)")
                        onClicked: launcher.goToPage("settings")
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: text
                    }
                    PlasmaComponents.ToolButton {
                        id: powerButton
                        visible: !launcher.compact
                        icon.name: "system-shutdown-symbolic"
                        display: PlasmaComponents.AbstractButton.IconOnly
                        text: i18n("Power and session")
                        onClicked: launcher.openMenu(launcher.powerEntries(), powerButton)
                        QQC2.ToolTip.visible: hovered && !menu.visible
                        QQC2.ToolTip.text: text
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Kirigami.Units.largeSpacing * 1.5
                opacity: launcher.contentProgress
                transform: Translate { y: (1 - launcher.contentProgress) * Kirigami.Units.gridUnit * 0.8 }

                ColumnLayout {
                    z: 2
                    Layout.fillHeight: true
                    Layout.preferredWidth: Kirigami.Units.gridUnit * (launcher.compact ? 3.6 : 4.4)
                    Layout.maximumWidth: Layout.preferredWidth
                    spacing: Kirigami.Units.smallSpacing

                    Component {
                        id: railButton
                        MouseArea {
                            id: railItem
                            required property var modelData
                            readonly property int index: launcher.pageDefs.findIndex(def => def.key === modelData.key)
                            readonly property bool current: !launcher.searching && launcher.page === modelData.key
                            readonly property int badge: modelData.key === "friends" ? launcherData.friendsInGame : 0
                            Layout.fillWidth: true
                            Layout.preferredHeight: Kirigami.Units.gridUnit * (launcher.compact ? 3 : 3.4)
                            hoverEnabled: true
                            property bool hintDismissed: false
                            onClicked: {
                                hintDismissed = true
                                launcher.goToPage(modelData.key)
                            }
                            onExited: hintDismissed = false

                            Rectangle {
                                anchors.fill: parent
                                radius: Kirigami.Units.cornerRadius * 2
                                color: railItem.current ? launcher.selectedFill : railItem.containsMouse ? launcher.hoverFill : "transparent"
                                border.width: railItem.current ? 1 : 0
                                border.color: launcher.hairline
                            }
                            Rectangle {
                                visible: railItem.badge > 0
                                anchors.top: parent.top
                                anchors.right: parent.right
                                anchors.margins: Kirigami.Units.smallSpacing
                                width: Math.max(height, badgeLabel.implicitWidth + Kirigami.Units.smallSpacing * 1.5)
                                height: badgeLabel.implicitHeight
                                radius: height / 2
                                color: Qt.alpha(Kirigami.Theme.positiveTextColor, 0.22)
                                border.width: 1
                                border.color: Qt.alpha(Kirigami.Theme.positiveTextColor, 0.55)
                                PlasmaComponents.Label {
                                    id: badgeLabel
                                    anchors.centerIn: parent
                                    text: railItem.badge
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                                    font.weight: Font.DemiBold
                                }
                            }
                            Rectangle {
                                visible: launcher.altHeld && railItem.index < 9
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.margins: Kirigami.Units.smallSpacing
                                width: Math.max(height, altKey.implicitWidth + Kirigami.Units.smallSpacing * 1.5)
                                height: altKey.implicitHeight + 2
                                radius: Kirigami.Units.cornerRadius
                                color: Qt.alpha(launcher.ink, 0.9)
                                PlasmaComponents.Label {
                                    id: altKey
                                    anchors.centerIn: parent
                                    text: railItem.index + 1
                                    color: Kirigami.Theme.backgroundColor
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                                    font.weight: Font.Bold
                                }
                            }
                            Timer {
                                id: hintDelay
                                interval: 450
                                running: railItem.containsMouse
                            }
                            Rectangle {
                                id: railHint
                                readonly property bool wanted: railItem.containsMouse && !hintDelay.running && !railItem.hintDismissed
                                visible: opacity > 0
                                opacity: wanted ? 1 : 0
                                Behavior on opacity { NumberAnimation { duration: 120 } }
                                x: parent.width + Kirigami.Units.largeSpacing
                                anchors.verticalCenter: parent.verticalCenter
                                width: hintRow.implicitWidth + Kirigami.Units.largeSpacing * 1.5
                                height: hintRow.implicitHeight + Kirigami.Units.smallSpacing * 2
                                radius: height / 2
                                color: Qt.rgba(0.08, 0.08, 0.09, 0.96)
                                border.width: 1
                                border.color: launcher.hairline
                                RowLayout {
                                    id: hintRow
                                    anchors.centerIn: parent
                                    spacing: Kirigami.Units.smallSpacing * 1.5
                                    PlasmaComponents.Label {
                                        text: railItem.badge > 0 ? i18np("%1 friend in game", "%1 friends in game", railItem.badge) : railItem.modelData.hint
                                        color: "white"
                                    }
                                    Rectangle {
                                        visible: railItem.index < 9
                                        implicitWidth: hintKey.implicitWidth + Kirigami.Units.smallSpacing * 1.5
                                        implicitHeight: hintKey.implicitHeight + 2
                                        radius: Kirigami.Units.cornerRadius
                                        color: Qt.alpha("white", 0.12)
                                        border.width: 1
                                        border.color: Qt.alpha("white", 0.18)
                                        PlasmaComponents.Label {
                                            id: hintKey
                                            anchors.centerIn: parent
                                            text: i18n("Alt %1", railItem.index + 1)
                                            color: "white"
                                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                                            opacity: 0.8
                                        }
                                    }
                                }
                            }
                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: Kirigami.Units.smallSpacing * 0.75
                                Kirigami.Icon {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                                    source: railItem.modelData.icon
                                    color: launcher.ink
                                    isMask: true
                                    opacity: railItem.current ? 1 : 0.62
                                }
                                PlasmaComponents.Label {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: railItem.modelData.label
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    font.weight: railItem.current ? Font.DemiBold : Font.Normal
                                    opacity: railItem.current ? 1 : 0.62
                                }
                            }
                        }
                    }
                    Repeater {
                        model: launcher.pageDefs.filter(def => def.key !== "settings")
                        delegate: railButton
                    }
                    Item { Layout.fillHeight: true }
                    Repeater {
                        model: launcher.pageDefs.filter(def => def.key === "settings")
                        delegate: railButton
                    }
                }

                Rectangle {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    color: launcher.hairline
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 0
                    Layout.minimumWidth: 0

                    Repeater {
                        id: pageLoaders
                        model: launcher.pageDefs
                        delegate: Loader {
                            required property var modelData
                            anchors.fill: parent
                            active: launcher.visited[modelData.key] === true || (launcher.warm && modelData.key === "apps")
                            asynchronous: !(launcher.page === modelData.key && !launcher.searching)
                            visible: !launcher.searching && launcher.page === modelData.key
                            source: Qt.resolvedUrl("pages/" + modelData.key.charAt(0).toUpperCase() + modelData.key.substring(1) + "Page.qml")
                            onLoaded: if (visible) Qt.callLater(launcher.resetSelection)
                        }
                    }

                    Loader {
                        id: searchLoader
                        anchors.fill: parent
                        active: launcher.searching || launcher.warm || item !== null
                        asynchronous: !launcher.searching
                        visible: launcher.searching
                        source: Qt.resolvedUrl("pages/SearchPage.qml")
                    }
                }
            }

            Rectangle {
                visible: !launcher.compact
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: launcher.hairline
                opacity: launcher.contentProgress
            }

            RowLayout {
                visible: !launcher.compact
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing * 2
                opacity: launcher.contentProgress

                Repeater {
                    model: launcher.searching ? [
                        { key: "↵", text: i18n("Open") },
                        { key: "Alt ↵", text: i18n("Actions") },
                        { key: "Tab", text: i18n("Next group") },
                        { key: "Esc", text: i18n("Clear") }
                    ] : [
                        { key: "↵", text: i18n("Open") },
                        { key: "Alt ↵", text: i18n("Actions") },
                        { key: "Ctrl P", text: i18n("Pin") },
                        { key: "Tab", text: i18n("Next group") },
                        { key: "Ctrl Tab", text: i18n("Next page") },
                        { key: "Alt 1–" + launcher.pageDefs.length, text: i18n("Go to page") },
                        { key: "Esc", text: i18n("Close") }
                    ]
                    delegate: RowLayout {
                        required property var modelData
                        spacing: Kirigami.Units.smallSpacing
                        Rectangle {
                            implicitWidth: keyLabel.implicitWidth + Kirigami.Units.largeSpacing
                            implicitHeight: keyLabel.implicitHeight + Kirigami.Units.smallSpacing * 0.5
                            radius: Kirigami.Units.cornerRadius
                            color: launcher.well
                            border.width: 1
                            border.color: launcher.hairline
                            PlasmaComponents.Label {
                                id: keyLabel
                                anchors.centerIn: parent
                                text: modelData.key
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                font.weight: Font.DemiBold
                                opacity: 0.8
                            }
                        }
                        PlasmaComponents.Label {
                            text: modelData.text
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.55
                        }
                    }
                }
                Item { Layout.fillWidth: true }
                PlasmaComponents.Label {
                    text: launcher.searching ? i18n("Prefixes: g games · a apps · f files · s packages · @ friends · = math · > command") : i18n("Type anywhere to search")
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    opacity: 0.45
                }
            }
        }

        QQC2.Menu {
            id: menu
            popupType: QQC2.Popup.Window

            property var entries: []

            onEntriesChanged: {
                while (count > 0)
                    takeItem(0).destroy()
                for (const entry of entries) {
                    if (entry.separator)
                        addItem(separatorComponent.createObject(null))
                    else
                        addItem(itemComponent.createObject(null, { text: entry.text, "icon.name": entry.icon || "", "icon.source": entry.iconSource || "", enabled: entry.disabled !== true, entry: entry }))
                }
            }
            onClosed: field.forceActiveFocus()
        }
    }

    Component {
        id: itemComponent
        PlasmaComponents.MenuItem {
            property var entry
            onTriggered: if (entry && entry.run) entry.run()
        }
    }
    Component {
        id: separatorComponent
        PlasmaComponents.MenuSeparator {}
    }
}
