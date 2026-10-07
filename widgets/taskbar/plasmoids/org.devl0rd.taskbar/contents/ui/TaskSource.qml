import QtQuick
import org.kde.taskmanager as TaskManager

QtObject {
    id: source

    property var pins: []
    property rect screenGeometry
    property bool filterLikePlasma: false
    property var windows: []
    property var launchers: ({})

    signal changed()

    function collect() {
        const list = []
        const found = {}
        for (let i = 0; i < rows.count; ++i) {
            const row = rows.objectAt(i)
            if (!row)
                continue
            if (row.isWindow)
                list.push(row.entry)
            else if (row.isLauncher)
                found[row.entry.appKey] = { url: row.entry.appKey, name: row.entry.appName, icon: row.entry.icon, index: row.index }
        }
        for (const window of list)
            if (!found[window.appKey])
                found[window.appKey] = { url: window.appKey, name: window.appName, icon: window.icon, index: -1, windowIndex: window.index }
        windows = list
        launchers = found
        changed()
    }

    function modelIndex(index) {
        return tasksModel.makeModelIndex(index)
    }
    function activate(window) {
        tasksModel.requestActivate(modelIndex(window.index))
    }
    function toggleMinimized(window) {
        tasksModel.requestToggleMinimized(modelIndex(window.index))
    }
    function close(window) {
        tasksModel.requestClose(modelIndex(window.index))
    }
    function newInstance(window) {
        tasksModel.requestNewInstance(modelIndex(window.index))
    }
    function launch(url) {
        const launcher = launchers[url]
        if (!launcher)
            return
        if (launcher.index >= 0)
            tasksModel.requestActivate(modelIndex(launcher.index))
        else
            tasksModel.requestNewInstance(modelIndex(launcher.windowIndex))
    }

    readonly property TaskManager.VirtualDesktopInfo desktops: TaskManager.VirtualDesktopInfo {}
    readonly property TaskManager.ActivityInfo activities: TaskManager.ActivityInfo {}

    readonly property TaskManager.TasksModel tasks: TaskManager.TasksModel {
        id: tasksModel
        groupMode: TaskManager.TasksModel.GroupDisabled
        sortMode: TaskManager.TasksModel.SortDisabled
        separateLaunchers: true
        launchInPlace: false
        launcherList: source.pins
        screenGeometry: source.screenGeometry
        virtualDesktop: source.desktops.currentDesktop
        activity: source.activities.currentActivity
        filterByActivity: true
        filterByScreen: source.filterLikePlasma
        filterByVirtualDesktop: source.filterLikePlasma
        filterNotMinimized: false
    }

    readonly property Instantiator rows: Instantiator {
        id: rows
        model: tasksModel
        delegate: QtObject {
            id: row
            required property int index
            required property var model
            readonly property bool isWindow: !!model.IsWindow && !model.IsStartup
            readonly property bool isLauncher: !!model.IsLauncher
            readonly property var winIds: model.WinIdList || []
            readonly property var entry: ({
                index: row.index,
                uuid: winIds.length > 0 ? String(winIds[0]) : "",
                appKey: String(model.LauncherUrlWithoutIcon || "") || String(model.AppId || ""),
                appId: String(model.AppId || ""),
                appName: String(model.AppName || ""),
                title: String(model.display || ""),
                icon: model.decoration,
                active: !!model.IsActive,
                minimized: !!model.IsMinimized,
                onCurrentDesktop: !!model.IsOnAllVirtualDesktops || (model.VirtualDesktops || []).indexOf(source.desktops.currentDesktop) >= 0,
                attention: !!model.IsDemandingAttention,
                lastActivated: model.LastActivated ? Number(model.LastActivated) : 0
            })
            onEntryChanged: Qt.callLater(source.collect)
            onIsWindowChanged: Qt.callLater(source.collect)
        }
        onObjectAdded: Qt.callLater(source.collect)
        onObjectRemoved: Qt.callLater(source.collect)
    }
}
