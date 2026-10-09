.pragma library

function windowRows(ids, windowsById, rowsByUuid) {
    const rows = []
    for (const id of ids) {
        const window = windowsById[id]
        const row = window ? rowsByUuid[window.uuid] : undefined
        if (row)
            rows.push(Object.assign({ konveyorId: id }, row))
    }
    return rows
}

function columnEntries(workspace, windowsById, rowsByUuid) {
    const entries = []
    workspace.columns.forEach((ids, position) => {
        const windows = windowRows(ids, windowsById, rowsByUuid)
        if (windows.length === 0)
            return
        entries.push({
            key: "w" + windows[0].uuid,
            kind: "column",
            appKey: windows[0].appKey,
            workspace: workspace.id,
            tabbed: (workspace.displays || [])[position] === "tabbed",
            columns: [{ index: position + 1, id: ids[0], focused: !!workspace.focused }],
            windows: windows
        })
    })
    return entries
}

function looseEntry(row, workspace) {
    return { key: "w" + row.uuid, kind: "window", appKey: row.appKey, workspace: workspace, tabbed: false, columns: [], windows: [row] }
}

function withParked(entries, rows, placed, parkedAfter, workspace) {
    const result = entries.slice()
    const loose = []
    let waiting = []
    for (const row of rows) {
        if (placed[row.uuid])
            continue
        const entry = looseEntry(row, workspace)
        if (row.uuid in parkedAfter)
            waiting.push({ entry: entry, after: parkedAfter[row.uuid] })
        else
            loose.push(entry)
    }
    const last = {}
    let progress = true
    while (waiting.length > 0 && progress) {
        progress = false
        const rest = []
        for (const item of waiting) {
            const key = item.after === null ? "" : item.after
            const anchor = item.after === null ? -1 : result.findIndex(each => each.windows.some(window => window.uuid === item.after))
            if (item.after !== null && anchor < 0) {
                rest.push(item)
                continue
            }
            const sibling = last[key] ? result.indexOf(last[key]) : -1
            result.splice(Math.max(anchor, sibling) + 1, 0, item.entry)
            last[key] = item.entry
            progress = true
        }
        waiting = rest
    }
    for (const item of waiting)
        result.push(item.entry)
    return result.concat(loose)
}

function mergeable(first, second) {
    return !!first && first.appKey && first.appKey === second.appKey && first.workspace === second.workspace
        && (first.kind === "group" || first.windows.length === 1) && second.windows.length === 1
        && (first.columns.length > 0) === (second.columns.length > 0)
}

function mergeAdjacent(entries) {
    const merged = []
    for (const entry of entries) {
        const last = merged[merged.length - 1]
        if (mergeable(last, entry)) {
            last.kind = "group"
            last.columns = last.columns.concat(entry.columns)
            last.windows = last.windows.concat(entry.windows)
        } else {
            merged.push(Object.assign({}, entry))
        }
    }
    return merged
}

function insertIdlePins(entries, pins, launchers) {
    const running = {}
    for (const entry of entries)
        for (const window of entry.windows)
            running[window.appKey] = true
    const inserted = {}
    const result = entries.slice()
    pins.forEach((pin, index) => {
        if (running[pin])
            return
        let anchor = ""
        for (let before = index - 1; before >= 0 && !anchor; --before)
            anchor = running[pins[before]] ? pins[before] : ""
        const count = inserted[anchor] || 0
        let at = count
        if (anchor) {
            let last = -1
            result.forEach((entry, position) => {
                if (entry.windows.some(window => window.appKey === anchor))
                    last = position
            })
            at = last + 1 + count
        }
        inserted[anchor] = count + 1
        result.splice(at, 0, { key: "p" + pin, kind: "pin", appKey: pin, workspace: -1, tabbed: false, columns: [], windows: [], pinned: true,
            launcher: launchers[pin] || { url: pin } })
    })
    return result
}

function buildItems(state) {
    const rowsByUuid = keyed(state.rows, row => row.uuid)
    let entries = []
    for (const workspace of state.workspaces || [])
        entries = entries.concat(columnEntries(workspace, state.windowsById, rowsByUuid))
    const placed = {}
    for (const entry of entries)
        for (const window of entry.windows)
            placed[window.uuid] = true
    const home = state.workspaces && state.workspaces.length > 0 ? state.workspaces[0].id : -1
    entries = withParked(entries, state.rows, placed, state.parkedAfter || {}, home)
    if (state.merge)
        entries = mergeAdjacent(entries)
    const pinned = keyed(state.pins, pin => pin)
    for (const entry of entries)
        entry.pinned = pinned[entry.appKey] !== undefined
    return insertIdlePins(entries, state.pins, state.launchers)
}

function columnBadge(entry, labels) {
    for (const column of (entry ? entry.columns : []))
        if (column.focused && labels[column.index])
            return labels[column.index]
    return ""
}

function mostRecent(windows) {
    return windows.reduce((best, window) => window.lastActivated > best.lastActivated ? window : best, windows[0])
}

function clickResult(windows, clicked, mode) {
    if (windows.length === 0)
        return { action: "launch" }
    if (clicked && !clicked.active)
        return { action: "activate", window: clicked }
    const active = clicked || windows.find(window => window.active)
    if (!active)
        return { action: "activate", window: mostRecent(windows) }
    if (mode === 1)
        return { action: "none" }
    if (clicked || windows.length === 1)
        return mode === 0 ? { action: "minimize", window: active } : { action: "cycle", window: active }
    return { action: "activate", window: windows[(windows.indexOf(active) + 1) % windows.length] }
}

function nextOfApp(windows, window) {
    const same = windows.filter(each => each.appKey === window.appKey)
    const at = same.findIndex(each => each.uuid === window.uuid)
    return same.length > 1 && at >= 0 ? same[(at + 1) % same.length] : null
}

function steppedWindow(windows, step) {
    if (windows.length === 0)
        return null
    const active = windows.findIndex(window => window.active)
    if (active < 0)
        return windows[step > 0 ? 0 : windows.length - 1]
    return windows[(active + step + windows.length) % windows.length]
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

function columnIds(entries) {
    const ids = []
    for (const entry of entries)
        for (const column of entry.columns)
            ids.push(column.id)
    return ids
}

function reorder(entries, from, to) {
    const moved = entries.slice()
    const entry = moved.splice(from, 1)[0]
    moved.splice(to, 0, entry)
    return moved
}

function pinOrder(entries, pins) {
    const order = []
    for (const entry of entries)
        if (pins.indexOf(entry.appKey) >= 0 && order.indexOf(entry.appKey) < 0)
            order.push(entry.appKey)
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

function shownWorkspaces(workspaces, showEmpty) {
    return workspaces.filter(workspace => showEmpty || workspace.is_active || (workspace.columns || []).length > 0)
}

function pinUrl(text) {
    const id = String(text || "").trim()
    if (!id)
        return ""
    if (/^[a-z]+:/.test(id))
        return id
    return "applications:" + (id.endsWith(".desktop") ? id : id + ".desktop")
}

function pinLabel(url) {
    return String(url).replace(/^applications:/, "").replace(/\.desktop$/, "").replace(/^file:\/\/.*\//, "")
}

function movedPin(pins, index, delta) {
    const target = index + delta
    if (index < 0 || target < 0 || target >= pins.length)
        return pins.slice()
    return reorder(pins, index, target)
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
