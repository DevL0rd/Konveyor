import QtQuick
import QtCore
import org.kde.plasma.plasma5support as P5Support
import "lib"

LauncherCatalogData {
    id: launcherData

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
}
