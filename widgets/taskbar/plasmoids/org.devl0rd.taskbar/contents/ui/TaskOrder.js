.pragma library

function columnItems(columns, windowsById, rowsByUuid) {
    const items = []
    columns.forEach((ids, position) => {
        const windows = []
        for (const id of ids) {
            const window = windowsById[id]
            const row = window ? rowsByUuid[window.uuid] : undefined
            if (row)
                windows.push(Object.assign({ konveyorId: id }, row))
        }
        if (windows.length > 0)
            items.push({ key: "c" + ids[0], appKey: windows[0].appKey, columns: [{ index: position + 1, id: ids[0] }], windows: windows })
    })
    return items
}

function withParked(items, rows, placed, parkedAfter) {
    const result = items.slice()
    const loose = []
    for (const row of rows) {
        if (placed[row.uuid])
            continue
        const item = { key: "w" + row.uuid, appKey: row.appKey, columns: [], windows: [row] }
        if (!(row.uuid in parkedAfter)) {
            loose.push(item)
            continue
        }
        const after = parkedAfter[row.uuid]
        const anchor = after === null ? -1 : result.findIndex(each => each.windows.some(window => window.konveyorId === after))
        result.splice(after !== null && anchor < 0 ? result.length : anchor + 1, 0, item)
    }
    return result.concat(loose)
}

function mergeAdjacent(items) {
    const merged = []
    for (const item of items) {
        const last = merged[merged.length - 1]
        if (last && last.appKey && last.appKey === item.appKey && (last.columns.length > 0) === (item.columns.length > 0)) {
            last.columns = last.columns.concat(item.columns)
            last.windows = last.windows.concat(item.windows)
        } else {
            merged.push(Object.assign({}, item))
        }
    }
    return merged
}

function insertIdlePins(items, pins, launchers) {
    const running = {}
    for (const item of items)
        running[item.appKey] = true
    const inserted = {}
    const result = items.slice()
    pins.forEach((pin, index) => {
        if (running[pin])
            return
        let anchor = ""
        for (let before = index - 1; before >= 0; --before) {
            if (running[pins[before]]) {
                anchor = pins[before]
                break
            }
        }
        const count = inserted[anchor] || 0
        let at = count
        if (anchor) {
            let last = -1
            result.forEach((item, position) => {
                if (item.appKey === anchor)
                    last = position
            })
            at = last + 1 + count
        }
        inserted[anchor] = count + 1
        result.splice(at, 0, { key: "p" + pin, appKey: pin, columns: [], windows: [], launcher: launchers[pin] || { url: pin } })
    })
    return result
}

function buildItems(state) {
    const rowsByUuid = {}
    for (const row of state.rows)
        rowsByUuid[row.uuid] = row
    const items = columnItems(state.columns, state.windowsById, rowsByUuid)
    const placed = {}
    for (const item of items)
        for (const window of item.windows)
            placed[window.uuid] = true
    let all = withParked(items, state.rows, placed, state.parkedAfter || {})
    if (state.merge)
        all = mergeAdjacent(all)
    const pinned = {}
    for (const pin of state.pins)
        pinned[pin] = true
    for (const item of all)
        item.pinned = !!pinned[item.appKey]
    return insertIdlePins(all, state.pins, state.launchers)
}

function planMoves(current, desired) {
    const order = current.slice()
    const moves = []
    desired.forEach((id, index) => {
        const from = order.indexOf(id)
        if (from < 0 || from === index)
            return
        order.splice(from, 1)
        order.splice(index, 0, id)
        moves.push({ id: id, index: index + 1 })
    })
    return moves
}

function columnIds(items) {
    const ids = []
    for (const item of items)
        for (const column of item.columns)
            ids.push(column.id)
    return ids
}

function reorder(items, from, to) {
    const moved = items.slice()
    const item = moved.splice(from, 1)[0]
    moved.splice(to, 0, item)
    return moved
}

function pinOrder(items, pins) {
    const order = []
    for (const item of items)
        if (pins.indexOf(item.appKey) >= 0 && order.indexOf(item.appKey) < 0)
            order.push(item.appKey)
    return order
}

function pinPlacement(appKeys, newIndex, pins) {
    const app = appKeys[newIndex]
    const pin = pins.indexOf(app)
    if (pin < 0 || appKeys.filter(key => key === app).length !== 1)
        return 0
    const others = appKeys.slice()
    others.splice(newIndex, 1)
    let target = -1
    for (let before = pin - 1; before >= 0 && target < 0; --before)
        target = others.lastIndexOf(pins[before]) >= 0 ? others.lastIndexOf(pins[before]) + 1 : -1
    for (let after = pin + 1; after < pins.length && target < 0; ++after)
        target = others.indexOf(pins[after])
    return target < 0 || target === newIndex ? 0 : target + 1
}

function shortKey(key, held) {
    return held.reduce((rest, modifier) => rest.replace(new RegExp("(^|\\+)" + modifier + "\\+"), "$1"), key)
}

function shortcutLabels(binds) {
    const labels = { workspaces: {}, columns: {} }
    for (const bind of binds) {
        const action = bind.action || {}
        const number = Number((action.arguments || [])[0])
        if (!Number.isInteger(number) || number < 1)
            continue
        const workspace = action.name === "focus-workspace"
        const table = workspace ? labels.workspaces : action.name === "focus-column" ? labels.columns : null
        if (table && !table[number])
            table[number] = shortKey(bind.key, workspace ? ["Super"] : ["Super", "Alt"])
    }
    return labels
}

function outputAt(outputs, screen) {
    const x = screen.x + screen.width / 2
    const y = screen.y + screen.height / 2
    const found = outputs.find(output => {
        const area = output.logical || {}
        return x >= area.x && x < area.x + area.width && y >= area.y && y < area.y + area.height
    })
    return found ? found.name : (outputs.length > 0 ? outputs[0].name : "")
}

function steppedWorkspace(workspaces, step) {
    const active = workspaces.findIndex(workspace => workspace.is_active)
    const next = Math.max(0, Math.min(workspaces.length - 1, active + step))
    return active < 0 || next === active ? null : workspaces[next]
}

function syncKeys(model, keys) {
    keys.forEach((key, index) => {
        let found = -1
        for (let i = index; i < model.count && found < 0; ++i)
            if (model.get(i).key === key)
                found = i
        if (found < 0)
            model.insert(index, { key: key })
        else if (found !== index)
            model.move(found, index, 1)
    })
    if (model.count > keys.length)
        model.remove(keys.length, model.count - keys.length)
}

function keyed(list, keyOf) {
    const table = {}
    for (const entry of list)
        table[keyOf(entry)] = entry
    return table
}
