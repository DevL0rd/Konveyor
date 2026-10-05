import QtQuick

LauncherState {
    id: launcher

    function hide() {
        launcherData.applet.hide()
    }
    function closeAndRun(action) {
        hide()
        action()
    }
    function launchGame(game) {
        if (!game || !game.launch)
            return
        remember("game:" + game.id)
        closeAndRun(() => launcherData.launchGame(game))
    }
    function openUrl(url) {
        closeAndRun(() => Qt.openUrlExternally(url))
    }
    function installPackage(pkg) {
        closeAndRun(() => launcherData.installPackage(pkg))
    }
    function sidebarToggleEntry(entry) {
        if (!entry)
            return []
        const pinned = launcherData.isOnSidebar(entry)
        return [{ text: pinned ? i18n("Unpin from sidebar") : i18n("Pin to sidebar"), icon: pinned ? "window-unpin" : "window-pin", run: () => launcherData.toggleSidebar(entry) }]
    }
    function openPin(pin) {
        if (!pin || pin.missing)
            return
        closeAndRun(() => launcherData.openSidebarPin(pin))
    }
    function sidebarEntries(pin, index) {
        const count = launcherData.sidebarPins.length
        const game = launcherData.sidebarGame(pin)
        const entries = []
        if (touchMode && !pin.missing) {
            entries.push({ text: pin.name, disabled: true })
            entries.push({ separator: true })
        }
        if (pin.missing) {
            entries.push({ text: pin.kind === "path" ? i18n("“%1” no longer exists", pin.name) : i18n("“%1” is not installed", pin.name), icon: "emblem-unavailable", disabled: true })
            entries.push({ text: i18n("Remove from sidebar"), icon: "edit-delete-remove", run: () => launcherData.removeSidebar(pin) })
        } else {
            entries.push({ text: game && game.launch ? i18n("Play") : i18n("Open"), icon: game && game.launch ? "media-playback-start" : pin.kind === "path" ? "document-open" : "system-run", run: () => launcher.openPin(pin) })
            if (pin.kind === "path" && pin.folder !== true)
                entries.push({ text: i18n("Open containing folder"), icon: "folder-open", run: () => launcher.closeAndRun(() => launcherData.showSidebarPinInFolder(pin)) })
            entries.push({ text: i18n("Unpin from sidebar"), icon: "window-unpin", run: () => launcherData.removeSidebar(pin) })
        }
        entries.push({ separator: true })
        entries.push({ text: i18n("Move up"), icon: "go-up-symbolic", disabled: index <= 0, run: () => launcherData.moveSidebar(index, index - 1) })
        entries.push({ text: i18n("Move down"), icon: "go-down-symbolic", disabled: index >= count - 1, run: () => launcherData.moveSidebar(index, index + 1) })
        return entries
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
        if (!model)
            return
        remember(key)
        if (key && String(key).indexOf(".desktop") >= 0)
            launcherData.trackApp(key)
        closeAndRun(() => model.trigger(index, "", null))
    }
    function kickerEntries(model, index, actions, favoriteId, url) {
        const entries = [{ text: i18n("Open"), icon: "system-run", run: () => launcher.trigger(model, index, favoriteId) }]
        const label = model && model.labelForRow ? model.labelForRow(index) : ""
        for (const entry of sidebarToggleEntry(launcherData.sidebarEntryFor(favoriteId, url, label)))
            entries.push(entry)
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
            if (!launcherData.applet.kickerApplet && favoriteId.indexOf(".desktop") >= 0) {
                entries.push({ text: i18n("Add to Panel (Widget)"), icon: "list-add", run: () => launcher.closeAndRun(() => launcherData.addLauncher("panel", favoriteId)) })
                entries.push({ text: i18n("Add to Desktop"), icon: "list-add", run: () => launcher.closeAndRun(() => launcherData.addLauncher("desktop", favoriteId)) })
            }
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
                run: () => launcher.closeAndRun(() => model.trigger(index, action.actionId, action.actionArgument))
            })
        }
        if (entries.length > 0 && entries[entries.length - 1].separator)
            entries.pop()
        return entries
    }
    function gameEntries(game) {
        const entries = [{ text: i18n("Play"), icon: "media-playback-start", run: () => launcher.launchGame(game) }]
        for (const entry of sidebarToggleEntry(launcherData.sidebarEntryForGame(game)))
            entries.push(entry)
        if (game.appid) {
            entries.push({ separator: true })
            entries.push({ text: i18n("Store page"), icon: "internet-web-browser", run: () => launcher.openUrl("steam://store/" + game.appid) })
            entries.push({ text: i18n("Properties"), icon: "configure", run: () => launcher.openUrl("steam://gameproperties/" + game.appid) })
            entries.push({ text: i18n("Browse local files"), icon: "folder-open", run: () => launcher.openUrl("steam://open/games/details/" + game.appid) })
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
            entries.push({ text: friend.name, icon: "im-user", iconSource: friend.avatar || "", run: () => launcher.openUrl(friend.chat) })
        }
        return entries
    }
    function friendEntries(friend) {
        const entries = [{ text: i18n("Open chat"), icon: "dialog-messages", run: () => launcher.openUrl(friend.chat) }]
        if (friend.join)
            entries.push({ text: i18n("Join game"), icon: "media-playback-start", run: () => launcher.openUrl(friend.join) })
        if (friend.ingame && friend.watch)
            entries.push({ text: i18n("Watch game"), icon: "view-visible", run: () => launcher.openUrl(friend.watch) })
        entries.push({ separator: true })
        entries.push({ text: i18n("View profile"), icon: "user-identity", run: () => launcher.openUrl(friend.profile) })
        if (friend.profile_web)
            entries.push({ text: i18n("Open profile in browser"), icon: "internet-web-browser", run: () => launcher.openUrl(friend.profile_web) })
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
            { text: i18n("Install with Shelly"), icon: "shelly", run: () => launcher.installPackage(pkg) },
            { separator: true },
            { text: pkg.source === "aur" ? i18n("Open AUR page") : i18n("Open package page"), icon: "internet-web-browser", run: () => launcher.openUrl(pkg.page) },
            { text: i18n("Open project website"), icon: "globe", disabled: !pkg.url, run: () => launcher.openUrl(pkg.url) },
            { text: i18n("Copy name"), icon: "edit-copy", run: () => launcherData.copyText(pkg.name) }
        ]
    }
    function openMenu(entries, item) {
        if (touchMode) {
            pendingMenu = { entries: entries, item: item }
            if (!touchDown)
                Qt.callLater(showPendingMenu)
            return
        }
        showMenu(entries, item)
    }
    function showPendingMenu() {
        if (!pendingMenu || touchDown)
            return
        const pending = pendingMenu
        pendingMenu = null
        showMenu(pending.entries, pending.item)
    }
    function showMenu(entries, item) {
        menu.entries = entries
        if (item)
            menu.popup(item, item.width / 2, item.height / 2)
        else
            menu.popup()
    }
}
