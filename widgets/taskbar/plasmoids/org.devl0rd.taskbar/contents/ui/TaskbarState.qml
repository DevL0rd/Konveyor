import QtQuick
import "TaskOrder.js" as TaskOrder

Item {
    id: state

    property var pins: []
    property rect screenGeometry
    property int groupMode: 0
    property bool placePinnedLaunches: true

    readonly property alias bus: konveyor
    readonly property alias source: tasks
    readonly property string output: TaskOrder.outputAt(konveyor.outputs, screenGeometry)
    readonly property var workspaces: konveyor.workspaces.filter(workspace => workspace.output === output).sort((a, b) => a.idx - b.idx)
    readonly property var activeWorkspace: workspaces.find(workspace => workspace.is_active) || null
    readonly property var labels: TaskOrder.shortcutLabels(konveyor.binds)
    readonly property bool merge: groupMode === 1 || (groupMode === 0 && activeWorkspace !== null && activeWorkspace.group_app_windows !== "off")
    property var items: []
    property var known: null
    property var pending: ({})
    property var places: ({})

    signal pinsEdited(var pins)
    signal launched(string appKey)

    function windowsById() {
        return TaskOrder.keyed(konveyor.windows, window => window.id)
    }

    function remember() {
        const next = {}
        for (const workspace of konveyor.workspaces) {
            workspace.columns.forEach((column, index) => {
                for (const id of column)
                    next[id] = { workspace: workspace.id, after: index > 0 ? workspace.columns[index - 1][0] : null }
            })
        }
        const byId = TaskOrder.keyed(konveyor.windows, window => window.id)
        const kept = {}
        const alive = TaskOrder.keyed(tasks.windows, row => row.uuid)
        for (const uuid in places)
            if (alive[uuid])
                kept[uuid] = places[uuid]
        for (const id in next)
            if (byId[id])
                kept[byId[id].uuid] = next[id]
        places = kept
    }

    function visibleRows() {
        if (!konveyor.available)
            return { rows: tasks.windows, parkedAfter: {} }
        const managed = TaskOrder.keyed(konveyor.windows, window => window.uuid)
        const workspaceId = activeWorkspace ? activeWorkspace.id : -1
        const parkedAfter = {}
        const rows = tasks.windows.filter(row => {
            if (managed[row.uuid] !== undefined)
                return managed[row.uuid].workspace_id === workspaceId
            const place = places[row.uuid]
            if (place && place.workspace === workspaceId)
                parkedAfter[row.uuid] = place.after
            return place ? place.workspace === workspaceId : row.minimized && row.onCurrentDesktop
        })
        return { rows: rows, parkedAfter: parkedAfter }
    }

    function rebuild() {
        const byId = windowsById()
        const visible = visibleRows()
        items = TaskOrder.buildItems({
            columns: konveyor.available && activeWorkspace ? activeWorkspace.columns : [],
            windowsById: byId,
            rows: visible.rows,
            parkedAfter: visible.parkedAfter,
            pins: pins,
            launchers: tasks.launchers,
            merge: merge
        })
        placePending(byId)
    }

    function noteNewWindows() {
        const ids = konveyor.windows.map(window => window.id)
        if (known !== null) {
            const added = Object.assign({}, pending)
            for (const id of ids)
                if (!known[id])
                    added[id] = Date.now()
            pending = added
        }
        known = TaskOrder.keyed(ids, id => id)
    }

    function placePending(byId) {
        if (!placePinnedLaunches || !activeWorkspace || Object.keys(pending).length === 0)
            return
        const rows = TaskOrder.keyed(tasks.windows, row => row.uuid)
        const columns = activeWorkspace.columns
        const appKeys = columns.map(column => {
            const window = byId[column[0]]
            const row = window ? rows[window.uuid] : undefined
            return row ? row.appKey : ""
        })
        const left = {}
        for (const id in pending) {
            const index = columns.findIndex(column => column.length === 1 && String(column[0]) === id)
            if (index >= 0 && appKeys[index]) {
                const target = TaskOrder.pinPlacement(appKeys, index, pins)
                if (target > 0)
                    konveyor.moveColumn(Number(id), target)
            } else if (Date.now() - pending[id] < 3000) {
                left[id] = pending[id]
            }
        }
        pending = left
    }

    function mostRecent(windows) {
        return windows.reduce((best, window) => window.lastActivated > best.lastActivated ? window : best, windows[0])
    }

    function activate(item) {
        const windows = item.windows
        if (windows.length === 0) {
            tasks.launch(item.appKey)
            launched(item.appKey)
            return
        }
        const current = windows.findIndex(window => window.active)
        if (current < 0)
            tasks.activate(mostRecent(windows))
        else if (windows.length === 1)
            tasks.toggleMinimized(windows[0])
        else
            tasks.activate(windows[(current + 1) % windows.length])
    }

    function close(item) {
        if (item.windows.length === 0)
            return
        const active = item.windows.find(window => window.active)
        tasks.close(active || mostRecent(item.windows))
    }

    function closeAll(item) {
        for (const window of item.windows)
            tasks.close(window)
    }

    function newInstance(item) {
        if (item.windows.length > 0)
            tasks.newInstance(item.windows[0])
        else
            tasks.launch(item.appKey)
    }

    function togglePin(item) {
        const next = pins.slice()
        const at = next.indexOf(item.appKey)
        if (at >= 0) {
            next.splice(at, 1)
        } else {
            const position = items.indexOf(item)
            const before = TaskOrder.pinOrder(items.slice(0, position), next)
            next.splice(before.length > 0 ? next.indexOf(before[before.length - 1]) + 1 : 0, 0, item.appKey)
        }
        pinsEdited(next)
    }

    function reorder(keys) {
        const byKey = TaskOrder.keyed(items, item => item.key)
        const ordered = keys.map(key => byKey[key]).filter(item => item)
        const moves = TaskOrder.planMoves(TaskOrder.columnIds(items), TaskOrder.columnIds(ordered))
        for (const move of moves)
            konveyor.moveColumn(move.id, move.index)
        const reordered = TaskOrder.pinOrder(ordered, pins)
        const untouched = pins.filter(pin => reordered.indexOf(pin) < 0)
        const nextPins = reordered.concat(untouched)
        if (nextPins.join("\n") !== pins.join("\n"))
            pinsEdited(nextPins)
        return moves.length
    }

    function switchTo(workspace) {
        if (workspace && !workspace.is_active)
            konveyor.focusWorkspace(output, workspace)
    }

    function step(delta) {
        switchTo(TaskOrder.steppedWorkspace(workspaces, delta))
    }

    onPinsChanged: rebuild()
    onMergeChanged: rebuild()
    onActiveWorkspaceChanged: rebuild()

    KonveyorBus {
        id: konveyor
        onLayoutRefreshed: {
            state.remember()
            state.noteNewWindows()
            state.rebuild()
        }
        onAvailableChanged: state.rebuild()
    }

    TaskSource {
        id: tasks
        pins: state.pins
        screenGeometry: state.screenGeometry
        filterLikePlasma: !konveyor.available
        onChanged: state.rebuild()
    }
}
