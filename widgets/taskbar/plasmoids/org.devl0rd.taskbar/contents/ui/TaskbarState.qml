import QtQuick
import org.kde.plasma.plasma5support as P5Support
import "TaskOrder.js" as TaskOrder

Item {
    id: state

    property var pins: []
    property rect screenGeometry
    property int groupMode: 0
    property bool placePinnedLaunches: true
    property bool onlyThisScreen: true
    property bool showFloating: true
    property int activeClick: 0
    property int middleClick: 0

    readonly property alias bus: konveyor
    readonly property alias source: tasks
    readonly property alias appActions: actions
    readonly property string output: TaskOrder.outputAt(konveyor.outputs, screenGeometry)
    readonly property var workspaces: konveyor.workspaces.filter(workspace => workspace.output === output).sort((a, b) => a.idx - b.idx)
    readonly property var activeWorkspace: workspaces.find(workspace => workspace.is_active) || null
    readonly property var taskWorkspaces: onlyThisScreen ? (activeWorkspace ? [activeWorkspace] : [])
        : konveyor.workspaces.filter(workspace => workspace.is_active).sort((a, b) => outputX(a.output) - outputX(b.output))
    readonly property var labels: TaskOrder.shortcutLabels(konveyor.binds)
    readonly property bool merge: groupMode === 1 || (groupMode === 0 && activeWorkspace !== null && activeWorkspace.group_app_windows !== "off")
    readonly property var allWindows: items.reduce((all, entry) => all.concat(entry.windows), [])
    property var items: []
    property var known: null
    property var pending: ({})
    property var places: ({})
    property var opening: null

    signal pinsEdited(var pins)

    function outputX(name) {
        const found = konveyor.outputs.find(each => each.name === name)
        return found && found.logical ? found.logical.x : 0
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
        const shown = TaskOrder.keyed(taskWorkspaces, workspace => workspace.id)
        const parkedAfter = {}
        const rows = tasks.windows.filter(row => {
            const window = managed[row.uuid]
            if (window !== undefined)
                return shown[window.workspace_id] !== undefined && (showFloating || !window.is_floating)
            const place = places[row.uuid]
            if (place && shown[place.workspace] !== undefined)
                parkedAfter[row.uuid] = place.after
            return place ? shown[place.workspace] !== undefined : row.minimized && row.onCurrentDesktop
        })
        return { rows: rows, parkedAfter: parkedAfter }
    }

    function rebuild() {
        const byId = TaskOrder.keyed(konveyor.windows, window => window.id)
        const visible = visibleRows()
        items = TaskOrder.buildItems({
            workspaces: konveyor.available ? taskWorkspaces.map(workspace => ({ id: workspace.id, columns: workspace.columns,
                displays: workspace.column_displays, focused: workspace.is_focused })) : [],
            windowsById: byId,
            rows: visible.rows,
            parkedAfter: visible.parkedAfter,
            pins: pins,
            launchers: tasks.launchers,
            merge: merge
        })
        placePending(byId)
        placeOpened(byId)
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

    function appKeyOf(id, byId) {
        const window = byId[id]
        const row = window ? tasks.windows.find(each => each.uuid === window.uuid) : undefined
        return row ? row.appKey : ""
    }

    function placePending(byId) {
        if (!placePinnedLaunches || !activeWorkspace || Object.keys(pending).length === 0)
            return
        const columns = activeWorkspace.columns
        const appKeys = columns.map(column => appKeyOf(column[0], byId))
        const left = {}
        for (const id in pending) {
            const index = columns.findIndex(column => column.length === 1 && String(column[0]) === id)
            if (index >= 0 && appKeys[index]) {
                const target = TaskOrder.pinPlacement(appKeys, index, pins)
                if (target > 0 && !(opening && opening.appKey === appKeys[index]))
                    konveyor.moveColumn(Number(id), target)
            } else if (Date.now() - pending[id] < 3000) {
                left[id] = pending[id]
            }
        }
        pending = left
    }

    function columnOf(id) {
        for (const workspace of konveyor.workspaces) {
            const index = workspace.columns.findIndex(column => column.indexOf(id) >= 0)
            if (index >= 0)
                return index + 1
        }
        return 0
    }

    function placeOpened(byId) {
        if (!opening)
            return
        if (Date.now() - opening.at > 5000) {
            opening = null
            return
        }
        const fresh = konveyor.windows.find(window => window.id !== opening.anchor && opening.before[window.id] === undefined
            && appKeyOf(window.id, byId) === opening.appKey)
        const anchor = columnOf(opening.anchor)
        if (!fresh || anchor === 0)
            return
        konveyor.moveColumn(fresh.id, anchor + 1)
        if (opening.row)
            konveyor.perform("consume-or-expel-window-left", [], fresh.id)
        opening = null
    }

    function openBeside(window, row) {
        opening = { appKey: window.appKey, anchor: window.konveyorId, row: row, at: Date.now(),
            before: TaskOrder.keyed(konveyor.windows, each => each.id) }
        tasks.newInstance(window)
    }

    function click(entry, window) {
        const result = TaskOrder.clickResult(entry.windows, window, activeClick)
        if (result.action === "launch")
            tasks.launch(entry.appKey)
        else if (result.action === "activate")
            tasks.activate(result.window)
        else if (result.action === "minimize")
            tasks.toggleMinimized(result.window)
        else if (result.action === "cycle")
            activateIfAny(TaskOrder.nextOfApp(allWindows, result.window))
        return result.action
    }

    function focusItem(number) {
        const entry = items[number - 1]
        if (!entry || !konveyor.ownsFocus(output))
            return
        const result = TaskOrder.itemShortcutResult(entry.windows)
        if (result.action === "launch")
            tasks.launch(entry.appKey)
        else
            tasks.activate(result.window)
    }

    function activateIfAny(window) {
        if (window)
            tasks.activate(window)
    }

    function targetOf(entry, window) {
        if (window)
            return window
        return entry.windows.length === 0 ? null : entry.windows.find(each => each.active) || TaskOrder.mostRecent(entry.windows)
    }

    function middle(entry, window) {
        const target = targetOf(entry, window)
        if (middleClick === 0 && target)
            tasks.close(target)
        else if (middleClick === 1)
            newInstance(entry)
    }

    function step(delta) {
        activateIfAny(TaskOrder.steppedWindow(allWindows, delta))
    }

    function newInstance(entry) {
        if (entry.windows.length > 0)
            tasks.newInstance(entry.windows[0])
        else
            tasks.launch(entry.appKey)
    }

    function togglePin(entry) {
        const next = pins.slice()
        const at = next.indexOf(entry.appKey)
        if (at >= 0) {
            next.splice(at, 1)
        } else {
            const position = items.indexOf(entry)
            const before = TaskOrder.pinOrder(items.slice(0, position), next)
            next.splice(before.length > 0 ? next.indexOf(before[before.length - 1]) + 1 : 0, 0, entry.appKey)
        }
        pinsEdited(next)
    }

    function menuContext(entry, window, rules) {
        const managed = window ? konveyor.windows.find(each => each.id === window.konveyorId) : null
        const home = managed ? konveyor.workspaces.find(each => each.id === managed.workspace_id) : null
        const index = home ? home.columns.findIndex(column => column.indexOf(managed.id) >= 0) : -1
        return {
            entry: entry,
            window: window,
            konveyor: !!home,
            column: index >= 0 ? { index: index + 1, count: home.columns.length, rows: home.columns[index].length } : null,
            tabbed: index >= 0 && (home.column_displays || [])[index] === "tabbed",
            floating: !!managed && managed.is_floating,
            workspaces: home ? konveyor.workspaces.filter(each => each.output === home.output).sort((a, b) => a.idx - b.idx)
                .map(each => ({ idx: each.idx, name: each.name || "", current: each.id === home.id })) : [],
            outputs: konveyor.outputs.map(each => each.name),
            output: home ? home.output : "",
            panelOutput: output,
            rules: rules,
            binds: konveyor.binds,
            appActions: appActions.appActions,
            player: appActions.playerState(),
            appName: window ? (window.appName || window.title) : ""
        }
    }

    function run(action, entry, window) {
        if (action.appAction) {
            appActions.trigger(action.appAction)
        } else if (action.media) {
            appActions.media(action.media)
        } else if (action.konveyor) {
            konveyor.perform(action.konveyor, action.args, window.konveyorId, action.konveyor.indexOf("move-window-to-") === 0 ? false : undefined)
        } else if (action.rule) {
            konveyor.setAppRule(window.konveyorId, action.rule, action.enabled)
        } else if (action.editRules) {
            const managed = konveyor.windows.find(each => each.id === window.konveyorId)
            commands.connectSource("kcmshell6 kcm_konveyor --args " + quoted("rules " + (managed ? managed.app_id : window.appId)))
        } else {
            runTask(action, entry, window)
        }
    }

    function runTask(action, entry, window) {
        switch (action.task) {
        case "activate":
            return tasks.activate(action.window)
        case "newInstance":
            return newInstance(entry)
        case "openRight":
            return openBeside(window, false)
        case "openBelow":
            return openBeside(window, true)
        case "togglePin":
            return togglePin(entry)
        case "allDesktops":
            return tasks.toggleAllDesktops(window)
        case "toggleMaximized":
            return tasks.toggleMaximized(window)
        case "toggleKeepAbove":
            return tasks.toggleKeepAbove(window)
        case "toggleKeepBelow":
            return tasks.toggleKeepBelow(window)
        case "toggleShaded":
            return tasks.toggleShaded(window)
        case "toggleMinimized":
            return tasks.toggleMinimized(window)
        case "closeAll":
            return entry.windows.forEach(each => tasks.close(each))
        default:
            return tasks.close(window)
        }
    }

    function quoted(text) {
        return "'" + String(text).replace(/'/g, "'\\''") + "'"
    }

    function reorder(keys) {
        const byKey = TaskOrder.keyed(items, entry => entry.key)
        const ordered = keys.map(key => byKey[key]).filter(entry => entry)
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

    function stepWorkspace(delta) {
        switchTo(TaskOrder.steppedWorkspace(workspaces, delta))
    }

    onPinsChanged: rebuild()
    onMergeChanged: rebuild()
    onTaskWorkspacesChanged: rebuild()
    onShowFloatingChanged: rebuild()

    KonveyorBus {
        id: konveyor
        onLayoutRefreshed: {
            state.remember()
            state.noteNewWindows()
            state.rebuild()
        }
        onAvailableChanged: state.rebuild()
        onTaskbarItemRequested: number => state.focusItem(number)
    }

    TaskSource {
        id: tasks
        pins: state.pins
        screenGeometry: state.screenGeometry
        filterLikePlasma: !konveyor.available
        onlyThisScreen: state.onlyThisScreen
        onChanged: state.rebuild()
    }

    AppActions {
        id: actions
    }

    P5Support.DataSource {
        id: commands
        engine: "executable"
        onNewData: source => disconnectSource(source)
    }
}
