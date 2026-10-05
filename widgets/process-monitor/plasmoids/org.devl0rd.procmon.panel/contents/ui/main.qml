import QtQuick
import QtQuick.Layouts
import QtQml.WorkerScript
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import org.kde.plasma.workspace.dbus as DBus
import org.kde.taskmanager as TaskManager
import "lib"
import "lib/PopStyle.js" as Style

ProcessFrameTimes {
    id: root

    readonly property bool overlayHost: Plasmoid.pluginName === "org.devl0rd.procmon.overlay"
    readonly property bool overlayVisible: overlayHost && monitorOverlay.targets.length > 0
    Plasmoid.status: overlayHost ? PlasmaCore.Types.HiddenStatus : PlasmaCore.Types.ActiveStatus
    readonly property string panelIcon: Plasmoid.configuration.panelIcon || "utilities-system-monitor"
    Plasmoid.icon: panelIcon
    Plasmoid.title: i18n("Process Monitor")

    readonly property int histLen: 40
    readonly property var histKeys: ["cpu", "ram", "gpu", "dec", "enc", "vram", "disk", "threads"]

    property var focusProc: null
    property var focusHistory: ({})
    property var summary: ({ count: 0, cpu: 0, gpu: 0, vram: 0, memTotal: 0, vramTotal: 0, gpuTop: null })
    property var procByPid: ({})
    property var sortHistByPid: ({})
    property var overlayProcByPid: ({})
    property var expandedRows: ({})
    property int expandedPid: 0
    property string searchText
    property var rowsView: null
    property bool sortSyncPending: false
    onRowsViewChanged: requestRebuild()
    readonly property int windowRows: 40
    property int windowStart: 0
    readonly property int windowEnd: windowStart + windowRows * 2
    function windowPids() {
        const pids = []
        const end = Math.min(rowModel.count, windowEnd + 1)
        for (let i = windowStart; i < end; ++i)
            pids.push(rowModel.get(i).pid)
        return pids
    }
    function updateWindow() {
        if (!rowsView)
            return
        const top = Math.floor(rowsView.contentY / (Kirigami.Units.gridUnit * 1.75))
        const wanted = Math.max(0, top - windowRows / 2)
        if (Math.abs(wanted - windowStart) >= windowRows / 2) {
            windowStart = wanted
            requestRebuild()
        }
    }
    property bool hasData: false

    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool dataWanted: overlayHost ? overlayVisible : (inPanel || visible)
    property bool popupAlive: !inPanel
    preferredRepresentation: inPanel ? compactRepresentation : fullRepresentation
    onPopupAliveChanged: requestRebuild()
    onExpandedChanged: function() {
        if (root.expanded) {
            releasePopup.stop()
            popupAlive = true
        } else if (inPanel) {
            releasePopup.restart()
        }
    }
    Timer {
        id: releasePopup
        interval: 1500
        onTriggered: root.popupAlive = root.expanded || !root.inPanel
    }


    TaskManager.TasksModel {
        id: tasks
        groupMode: TaskManager.TasksModel.GroupDisabled
        filterByVirtualDesktop: false
        filterByActivity: false
        filterByScreen: false
        filterNotMinimized: false
    }
    readonly property int activePid: {
        tasks.count
        const pid = tasks.activeTask ? tasks.data(tasks.activeTask, TaskManager.AbstractTasksModel.AppPid) : 0
        return pid > 0 ? pid : 0
    }
    readonly property string activeAppName: {
        tasks.count
        const name = tasks.activeTask ? tasks.data(tasks.activeTask, TaskManager.AbstractTasksModel.AppName) : ""
        return name ? String(name) : ""
    }
    property int lastActivePid: 0
    property string lastActiveAppName: ""
    onActivePidChanged: if (activePid > 0) {
        lastActivePid = activePid
        lastActiveAppName = activeAppName
    }
    onActiveAppNameChanged: if (activePid > 0) lastActiveAppName = activeAppName
    readonly property int focusPid: activePid > 0 ? activePid : lastActivePid
    readonly property string focusAppName: activePid > 0 ? activeAppName : lastActiveAppName
    onFocusPidChanged: {
        writeFocus()
        requestRebuild()
    }
    onFocusAppNameChanged: writeFocus()

    DBus.SignalWatcher {
        enabled: root.dataWanted
        busType: DBus.BusType.Session
        service: "org.devl0rd.ProcessMonitor.FrameTelemetry"
        path: "/FrameTelemetry"
        iface: "org.devl0rd.ProcessMonitor.FrameTelemetry"

        function dbusFrame(pid, frametime) {
            root.recordFrametime(Number(pid), Number(frametime))
        }
    }

    function writeFocus() {
        if (runtimeDir)
            run("printf '%s\\n%s\\n' " + focusPid + " " + shq(focusAppName) + " > " + shq(runtimeDir + "/focus"))
    }

    property string runtimeDir
    readonly property string cachePath: runtimeDir ? runtimeDir + "/data.json" : ""
    readonly property string panelPath: runtimeDir ? runtimeDir + "/panel/panel.json" : ""
    readonly property bool compactOnly: inPanel && !popupAlive && monitorOverlay.pids.length === 0
    onCompactOnlyChanged: read()
    P5Support.DataSource {
        id: pathHelper
        engine: "executable"
        onNewData: function(source, data) {
            root.runtimeDir = (data.stdout || "").trim()
            disconnectSource(source)
            root.writeFocus()
            root.read()
        }
    }
    function read() {
        if (!runtimeDir || !worker.ready)
            return
        if (compactOnly)
            worker.sendMessage({ panel: panelPath, state: workerState() })
        else
            worker.sendMessage({ path: cachePath, state: workerState() })
    }
    FileWatcher {
        path: root.dataWanted ? root.cachePath : ""
        onChanged: {
            watchdog.beat()
            if (!root.compactOnly)
                root.read()
        }
    }
    readonly property bool collectorStale: watchdog.stale
    CollectorWatchdog {
        id: watchdog
        active: root.dataWanted && root.runtimeDir !== ""
        staleAfter: Math.max(2000, Plasmoid.configuration.updateInterval * 4)
        restartCommand: "$HOME/.local/bin/procmon-collect --restart"
    }
    FileWatcher {
        path: root.dataWanted && root.compactOnly ? root.panelPath : ""
        onChanged: root.read()
    }
    function workerState() {
        const sortColumn = Plasmoid.configuration.sortColumn || "cpu"
        const column = colOf(sortColumn)
        return {
            histKeys: histKeys, histLen: histLen, aggregate: Plasmoid.configuration.aggregateChildren,
            sortColumn: sortColumn, sortNoagg: column ? !!column.noagg : true, sortDescending: Plasmoid.configuration.sortDescending,
            searchText: searchText.trim(), showKernel: Plasmoid.configuration.showKernelThreads, hideSystemd: Plasmoid.configuration.hideSystemd,
            expanded: expandedRows, tree: Plasmoid.configuration.treeView, filter: Plasmoid.configuration.processFilter,
            focusPid: focusPid, full: rowsView !== null,
            overlayPids: monitorOverlay.pids,
            windowStart: windowStart, windowEnd: windowEnd, windowPids: windowPids()
        }
    }
    function requestRebuild() {
        if (worker.ready)
            worker.sendMessage({ text: null, state: workerState() })
    }
    WorkerScript {
        id: worker
        source: Qt.resolvedUrl("proc.worker.mjs")
        onReadyChanged: if (ready) root.read()
        property string compactSig
        onMessage: function(message) {
            if (!root.hasData)
                root.hasData = true
            if (!message.full) {
                if (message.overlayProcByPid)
                    root.overlayProcByPid = message.overlayProcByPid
                if (message.compactSig === compactSig && !message.overlay)
                    return
                compactSig = message.compactSig
                root.focusProc = message.focus || root.focusProc
                root.summary = message.summary
                return
            }
            compactSig = ""
            root.focusProc = message.focus || root.focusProc
            root.focusHistory = message.focusHistory
            root.summary = message.summary
            root.overlayProcByPid = message.overlayProcByPid || ({})
            root.procByPid = message.procByPid
            root.sortHistByPid = message.sortHistByPid
            if (!message.desired)
                return
            if (root.sortSyncPending) {
                root.syncModel(message.desired)
                root.sortSyncPending = false
                if (root.rowsView)
                    root.rowsView.positionViewAtBeginning()
            } else if (!root.rowsView || root.rowsView.contentY < Kirigami.Units.gridUnit * 1.7) {
                root.syncModel(message.desired)
            } else {
                root.syncFrozen(message.desired)
            }
        }
    }
    Connections {
        target: Plasmoid.configuration
        function onUpdateIntervalChanged() { root.applyInterval() }
        function onAggregateChildrenChanged() { root.requestRebuild() }
        function onHideSystemdChanged() { root.requestRebuild() }
        function onShowKernelThreadsChanged() { root.requestRebuild() }
        function onSortColumnChanged() { root.requestRebuild() }
        function onSortDescendingChanged() { root.requestRebuild() }
        function onTreeViewChanged() { root.requestRebuild() }
        function onProcessFilterChanged() { root.requestRebuild() }
    }
    onSearchTextChanged: requestRebuild()
    Component.onCompleted: {
        pathHelper.connectSource("printf %s \"$XDG_RUNTIME_DIR/Linux-Process-Mon\"")
        applyInterval()
        syncFrametimeWatch()
    }
    Component.onDestruction: {
        const message = {
            service: "org.devl0rd.ProcessMonitor.FrameTelemetry",
            path: "/FrameTelemetry",
            iface: "org.devl0rd.ProcessMonitor.FrameTelemetry",
            member: "Watch",
            arguments: ["[]"]
        }
        DBus.SessionBus.asyncCall(message)
    }
    function applyInterval() {
        run("$HOME/.local/bin/procmon-collect --set-interval " + (Math.max(500, Plasmoid.configuration.updateInterval) / 1000))
    }

    ListModel { id: rowModel }
    readonly property alias rows: rowModel

    function syncFrozen(desired) {
        const want = {}, map = {}
        for (const row of desired) {
            want[row.pid] = true
            map[row.pid] = row
        }
        for (let r = rowModel.count - 1; r >= 0; --r) {
            if (want[rowModel.get(r).pid] !== true)
                rowModel.remove(r)
        }
        const have = {}
        for (let x = 0; x < rowModel.count; ++x) {
            const current = rowModel.get(x)
            have[current.pid] = true
            const next = map[current.pid]
            if (next && (current.depth !== next.depth || current.hasChildren !== next.hasChildren || current.expanded !== next.expanded))
                rowModel.set(x, next)
        }
        for (const row of desired) {
            if (have[row.pid] !== true)
                rowModel.append(row)
        }
    }
    function syncModel(desired) {
        const want = {}
        for (const row of desired)
            want[row.pid] = true
        for (let r = rowModel.count - 1; r >= 0; --r) {
            if (want[rowModel.get(r).pid] !== true)
                rowModel.remove(r)
        }
        for (let pos = 0; pos < desired.length; ++pos) {
            const row = desired[pos]
            if (pos < rowModel.count && rowModel.get(pos).pid === row.pid) {
                const current = rowModel.get(pos)
                if (current.depth !== row.depth || current.hasChildren !== row.hasChildren || current.expanded !== row.expanded)
                    rowModel.set(pos, row)
                continue
            }
            let found = -1
            for (let x = pos + 1; x < rowModel.count; ++x) {
                if (rowModel.get(x).pid === row.pid) {
                    found = x
                    break
                }
            }
            if (found < 0) {
                rowModel.insert(pos, row)
            } else {
                rowModel.move(found, pos, 1)
                const moved = rowModel.get(pos)
                if (moved.depth !== row.depth || moved.hasChildren !== row.hasChildren || moved.expanded !== row.expanded)
                    rowModel.set(pos, row)
            }
        }
    }
    function toggleTree(pid) {
        const next = Object.assign({}, expandedRows)
        next[pid] = !next[pid]
        expandedRows = next
        requestRebuild()
    }
    function collapseAll() {
        expandedRows = ({})
        requestRebuild()
    }
    function headerSort(key) {
        const descending = Plasmoid.configuration.sortColumn === key ? !Plasmoid.configuration.sortDescending : key !== "name"
        sortSyncPending = true
        windowStart = 0
        if (rowsView)
            rowsView.positionViewAtBeginning()
        Plasmoid.configuration.sortColumn = key
        Plasmoid.configuration.sortDescending = descending
    }

    function revealFocused() {
        if (!focusProc)
            return
        expandedPid = focusProc.pid
        searchText = String(focusProc.pid)
        expanded = true
    }
    function middleClick() {
        revealFocused()
    }

    readonly property string focusName: focusProc ? focusProc.name : ""

    toolTipMainText: focusProc ? focusName + " · PID " + focusProc.pid : i18n("Process Monitor")
    property bool tooltipWanted: false
    function tooltipText() {
        if (collectorStale)
            return i18n("Collector stopped")
        if (!focusProc)
            return hasData ? i18n("%1 processes", summary.count) : i18n("Waiting for the collector")
        const lines = [i18n("CPU %1% · GPU %2% · VRAM %3 · RAM %4", Math.round(focusProc.cpu), Math.round(focusProc.gpu), Style.bytes(focusProc.vram), Style.bytes(focusProc.ram))]
        if (focusProc.fps >= 0)
            lines.push(i18n("%1 FPS · %2 ms · 1% low %3", focusProc.fps, focusProc.frametime.toFixed(1), focusProc.fpsLow))
        return lines.join("\n")
    }
    toolTipSubText: tooltipWanted ? tooltipText() : ""

    compactRepresentation: CompactView {}
    fullRepresentation: FullView {}

    MonitorOverlay {
        id: monitorOverlay
        active: root.overlayHost
        slot: 1
        content: Component { CompactView {} }
        popupContent: Component { FullView {} }
        onPidsChanged: root.read()
    }
}
