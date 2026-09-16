import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.layershell as LayerShell

Window {
    id: launcher

    readonly property bool wanted: root.open
    property real progress: 0
    property real contentProgress: 0
    property bool hadFocus: false
    property string page: "home"
    property var visited: ({ home: true })
    property int sectionIndex: 0

    readonly property string rawQuery: field.text
    readonly property string mode: {
        const text = rawQuery
        if (text.startsWith("g ")) return "games"
        if (text.startsWith("f ")) return "files"
        if (text.startsWith("a ")) return "apps"
        if (text.startsWith("@")) return "friends"
        if (text.startsWith("=")) return "calc"
        if (text.startsWith(">")) return "command"
        return "all"
    }
    readonly property string term: {
        const text = rawQuery
        if (mode === "games" || mode === "files" || mode === "apps") return text.substring(2).trim()
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
            { key: "home", label: i18n("Home"), icon: "go-home-symbolic" },
            { key: "apps", label: i18n("Apps"), icon: "view-app-grid-symbolic" }
        ]
        if (Plasmoid.configuration.showGames)
            defs.push({ key: "games", label: i18n("Games"), icon: "input-gamepad-symbolic" })
        defs.push({ key: "files", label: i18n("Files"), icon: "folder-documents-symbolic" })
        if (Plasmoid.configuration.showFriends)
            defs.push({ key: "friends", label: i18n("Friends"), icon: "system-users-symbolic" })
        defs.push({ key: "system", label: i18n("System"), icon: "system-shutdown-symbolic" })
        return defs
    }
    readonly property int pageIndex: Math.max(0, pageDefs.findIndex(def => def.key === page))

    LauncherData {
        id: launcherData
        applet: root
        live: launcher.visible
        query: launcher.term
        searchMode: launcher.mode
    }

    visible: false
    color: "transparent"
    flags: Qt.FramelessWindowHint
    title: i18n("Portal Launcher")

    LayerShell.Window.scope: "portal-launcher"
    LayerShell.Window.layer: LayerShell.Window.LayerOverlay
    LayerShell.Window.anchors: LayerShell.Window.AnchorTop | LayerShell.Window.AnchorBottom | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
    LayerShell.Window.exclusionZone: -1
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityExclusive
    LayerShell.Window.wantsToBeOnActiveScreen: root.openScreen === ""

    onWantedChanged: wanted ? openNow() : closeNow()
    Component.onCompleted: if (wanted) openNow()

    function openNow() {
        closeAnimation.stop()
        const target = Qt.application.screens.find(entry => entry.name === root.openScreen)
        if (target)
            screen = target
        page = pageDefs.some(def => def.key === Plasmoid.configuration.defaultPage) ? Plasmoid.configuration.defaultPage : "home"
        markVisited(page)
        field.text = ""
        hadFocus = false
        visible = true
        requestActivate()
        field.forceActiveFocus()
        openAnimation.restart()
        Qt.callLater(resetSelection)
    }
    function closeNow() {
        openAnimation.stop()
        menu.close()
        closeAnimation.restart()
    }
    onActiveChanged: {
        if (active)
            hadFocus = true
        else if (hadFocus && visible && root.open && !menu.visible)
            root.hide()
    }

    ParallelAnimation {
        id: openAnimation
        NumberAnimation { target: launcher; property: "progress"; to: 1; duration: Kirigami.Units.longDuration * 1.6; easing.type: Easing.OutCubic }
        SequentialAnimation {
            PauseAnimation { duration: Kirigami.Units.shortDuration * 0.6 }
            NumberAnimation { target: launcher; property: "contentProgress"; to: 1; duration: Kirigami.Units.longDuration * 1.4; easing.type: Easing.OutCubic }
        }
    }
    SequentialAnimation {
        id: closeAnimation
        ParallelAnimation {
            NumberAnimation { target: launcher; property: "progress"; to: 0; duration: Kirigami.Units.longDuration; easing.type: Easing.InCubic }
            NumberAnimation { target: launcher; property: "contentProgress"; to: 0; duration: Kirigami.Units.shortDuration; easing.type: Easing.InCubic }
        }
        ScriptAction {
            script: {
                launcher.visible = false
                field.text = ""
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
    function goToPage(key) {
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
    function trigger(model, index) {
        if (model && model.trigger(index, "", null) !== false)
            root.hide()
    }
    function kickerEntries(model, index, actions, favoriteId) {
        const entries = [{ text: i18n("Open"), icon: "system-run", run: () => launcher.trigger(model, index) }]
        if (favoriteId) {
            const pinned = isPinned(favoriteId)
            entries.push({ text: pinned ? i18n("Unpin from Home") : i18n("Pin to Home"), icon: pinned ? "window-unpin" : "window-pin", run: () => launcher.togglePin(favoriteId) })
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
        const entries = [{ text: i18n("Play"), icon: "media-playback-start", run: () => { launcherData.launchGame(game); root.hide() } }]
        if (game.appid) {
            entries.push({ separator: true })
            entries.push({ text: i18n("Store page"), icon: "internet-web-browser", run: () => { Qt.openUrlExternally("steam://store/" + game.appid); root.hide() } })
            entries.push({ text: i18n("Properties"), icon: "configure", run: () => { Qt.openUrlExternally("steam://gameproperties/" + game.appid); root.hide() } })
            entries.push({ text: i18n("Browse local files"), icon: "folder-open", run: () => { Qt.openUrlExternally("steam://open/games/details/" + game.appid); root.hide() } })
        }
        for (const friend of launcherData.friendsFor(game)) {
            if (entries.length === 1 || !entries[entries.length - 1].friendHeader) {
                entries.push({ separator: true })
                entries.push({ text: i18n("Playing now"), friendHeader: true, disabled: true })
            }
            entries.push({ text: friend.name, icon: "im-user", run: () => { Qt.openUrlExternally(friend.chat); root.hide() } })
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
    function openMenu(entries, item) {
        menu.entries = entries
        if (item)
            menu.popup(item, item.width / 2, item.height / 2)
        else
            menu.popup()
    }

    Item {
        id: scene
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: launcher.progress * Plasmoid.configuration.dimStrength
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
            onClicked: root.hide()
        }

        Rectangle {
            id: card

            width: Math.min(parent.width - Kirigami.Units.gridUnit * 4, Math.max(Kirigami.Units.gridUnit * 40, parent.width * Plasmoid.configuration.widthFraction))
            height: Math.min(parent.height - Kirigami.Units.gridUnit * 4, Math.max(Kirigami.Units.gridUnit * 30, parent.height * Plasmoid.configuration.heightFraction))
            anchors.centerIn: parent
            anchors.verticalCenterOffset: (1 - launcher.progress) * Kirigami.Units.gridUnit * 2
            scale: 0.94 + 0.06 * launcher.progress
            opacity: launcher.progress
            radius: Kirigami.Units.cornerRadius * 4
            color: Kirigami.Theme.backgroundColor
            border.width: 1
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.12)

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.gridUnit
                spacing: Kirigami.Units.largeSpacing

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing

                    Components.Avatar {
                        Layout.preferredWidth: Kirigami.Units.iconSizes.large
                        Layout.preferredHeight: Kirigami.Units.iconSizes.large
                        source: launcherData.user.faceIconUrl
                        name: launcherData.user.fullName || launcherData.user.loginName
                    }
                    ColumnLayout {
                        spacing: 0
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 9
                        Layout.maximumWidth: Kirigami.Units.gridUnit * 9
                        Kirigami.Heading {
                            level: 3
                            text: launcherData.user.fullName || launcherData.user.loginName
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        PlasmaComponents.Label {
                            text: launcherData.user.loginName + "@" + launcherData.user.host
                            font: Kirigami.Theme.smallFont
                            opacity: 0.6
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 2.4
                        radius: height / 2
                        color: Qt.alpha(Kirigami.Theme.textColor, 0.06)
                        border.width: field.activeFocus ? 1 : 0
                        border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.7)

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Kirigami.Units.largeSpacing
                            anchors.rightMargin: Kirigami.Units.largeSpacing
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Icon {
                                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                                source: "search"
                                opacity: 0.7
                            }
                            QQC2.TextField {
                                id: field
                                Layout.fillWidth: true
                                Layout.preferredWidth: 0
                                background: null
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.25
                                placeholderText: i18n("Search apps, games, files, settings, friends…")
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
                                    } else if (ctrl && event.key === Qt.Key_P) {
                                        launcher.pinCurrent()
                                    } else if (ctrl && event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
                                        const at = event.key - Qt.Key_1
                                        if (at < launcherData.favorites.count)
                                            launcher.trigger(launcherData.favorites, at)
                                    } else {
                                        return
                                    }
                                    event.accepted = true
                                }
                            }
                            PlasmaComponents.Label {
                                visible: launcher.searching && launcher.mode !== "all"
                                text: ({ games: i18n("Games"), files: i18n("Files"), apps: i18n("Apps"), friends: i18n("Friends"), calc: i18n("Calculator"), command: i18n("Command") })[launcher.mode] || ""
                                font: Kirigami.Theme.smallFont
                                color: Kirigami.Theme.highlightColor
                            }
                            PlasmaComponents.ToolButton {
                                visible: field.text !== ""
                                icon.name: "edit-clear"
                                display: PlasmaComponents.AbstractButton.IconOnly
                                text: i18n("Clear")
                                onClicked: {
                                    field.text = ""
                                    field.forceActiveFocus()
                                }
                            }
                        }
                    }

                    PlasmaComponents.ToolButton {
                        icon.name: "configure"
                        display: PlasmaComponents.AbstractButton.IconOnly
                        text: i18n("Configure…")
                        onClicked: {
                            root.hide()
                            Plasmoid.internalAction("configure").trigger()
                        }
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: text
                    }
                    PlasmaComponents.ToolButton {
                        id: powerButton
                        icon.name: "system-shutdown"
                        display: PlasmaComponents.AbstractButton.IconOnly
                        text: i18n("Power and session")
                        onClicked: launcher.goToPage("system")
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: text
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: Kirigami.Units.largeSpacing
                    opacity: launcher.contentProgress
                    transform: Translate { y: (1 - launcher.contentProgress) * Kirigami.Units.gridUnit }

                    ColumnLayout {
                        Layout.fillHeight: true
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 4.6
                        Layout.maximumWidth: Kirigami.Units.gridUnit * 4.6
                        spacing: Kirigami.Units.smallSpacing

                        Repeater {
                            model: launcher.pageDefs
                            delegate: MouseArea {
                                id: railItem
                                required property var modelData
                                required property int index
                                readonly property bool current: !launcher.searching && launcher.page === modelData.key
                                Layout.fillWidth: true
                                Layout.preferredHeight: Kirigami.Units.gridUnit * 3.6
                                hoverEnabled: true
                                onClicked: launcher.goToPage(modelData.key)

                                Rectangle {
                                    anchors.fill: parent
                                    radius: Kirigami.Units.cornerRadius * 2
                                    color: railItem.current ? Qt.alpha(Kirigami.Theme.highlightColor, 0.2)
                                         : railItem.containsMouse ? Qt.alpha(Kirigami.Theme.textColor, 0.07) : "transparent"
                                    Behavior on color { ColorAnimation { duration: 120 } }
                                }
                                Rectangle {
                                    visible: railItem.current
                                    width: 3
                                    height: parent.height * 0.5
                                    radius: 1.5
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: Kirigami.Theme.highlightColor
                                }
                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 2
                                    Kirigami.Icon {
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                        Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                                        source: railItem.modelData.icon
                                        color: railItem.current ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
                                        isMask: true
                                    }
                                    PlasmaComponents.Label {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: railItem.modelData.label
                                        font: Kirigami.Theme.smallFont
                                        opacity: railItem.current ? 1 : 0.7
                                    }
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }

                    Kirigami.Separator {
                        Layout.fillHeight: true
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Repeater {
                            id: pageLoaders
                            model: launcher.pageDefs
                            delegate: Loader {
                                required property var modelData
                                anchors.fill: parent
                                active: launcher.visited[modelData.key] === true
                                visible: !launcher.searching && launcher.page === modelData.key
                                source: Qt.resolvedUrl("pages/" + modelData.key.charAt(0).toUpperCase() + modelData.key.substring(1) + "Page.qml")
                                onLoaded: if (visible) Qt.callLater(launcher.resetSelection)
                            }
                        }

                        Loader {
                            id: searchLoader
                            anchors.fill: parent
                            active: launcher.searching || item !== null
                            visible: launcher.searching
                            source: Qt.resolvedUrl("pages/SearchPage.qml")
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing * 2
                    opacity: 0.55 * launcher.contentProgress

                    Repeater {
                        model: launcher.searching ? [
                            { key: "↵", text: i18n("Open") },
                            { key: "Alt+↵", text: i18n("Actions") },
                            { key: "Ctrl+P", text: i18n("Pin") },
                            { key: "Tab", text: i18n("Next group") },
                            { key: "g · f · a · @ · = · >", text: i18n("Search games, files, apps, friends, math, commands") }
                        ] : [
                            { key: "↵", text: i18n("Open") },
                            { key: "Alt+↵", text: i18n("Actions") },
                            { key: "Ctrl+P", text: i18n("Pin") },
                            { key: "Ctrl+1…9", text: i18n("Pinned apps") },
                            { key: "Ctrl+Tab", text: i18n("Pages") },
                            { key: "Esc", text: i18n("Close") }
                        ]
                        delegate: RowLayout {
                            required property var modelData
                            spacing: Kirigami.Units.smallSpacing
                            Rectangle {
                                implicitWidth: keyLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                                implicitHeight: keyLabel.implicitHeight + 2
                                radius: Kirigami.Units.cornerRadius
                                color: Qt.alpha(Kirigami.Theme.textColor, 0.1)
                                PlasmaComponents.Label {
                                    id: keyLabel
                                    anchors.centerIn: parent
                                    text: modelData.key
                                    font: Kirigami.Theme.smallFont
                                }
                            }
                            PlasmaComponents.Label {
                                text: modelData.text
                                font: Kirigami.Theme.smallFont
                            }
                        }
                    }
                    Item { Layout.fillWidth: true }
                }
            }
        }
    }

    QQC2.Menu {
        id: menu

        property var entries: []

        onEntriesChanged: {
            while (count > 0)
                takeItem(0).destroy()
            for (const entry of entries) {
                if (entry.separator)
                    addItem(separatorComponent.createObject(null))
                else
                    addItem(itemComponent.createObject(null, { text: entry.text, "icon.name": entry.icon || "", enabled: entry.disabled !== true, entry: entry }))
            }
        }
        onClosed: field.forceActiveFocus()
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
