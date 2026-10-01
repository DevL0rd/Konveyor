function key(value) {
    return String(value).replace(/[{}]/g, "").toLowerCase()
}

function entry(row) {
    return row.entry || row
}

function scoped(windows, output, workspace) {
    return windows.filter(window => window.output === output && window.workspace_id === workspace)
}

function byTask(windows) {
    const result = {}
    for (const window of windows)
        result[key(window.task_id)] = window
    return result
}

function grouping(entries, windows, mode, fallback) {
    if (mode !== "follow")
        return {grouped: mode === "grouped", blacklist: []}
    const tasks = byTask(windows)
    const blacklist = []
    const byApp = {}
    let grouped = fallback !== 0
    for (const row of entries) {
        const item = entry(row)
        const modes = byApp[item.appId] || new Set()
        for (const id of item.windowIds) {
            const window = tasks[key(id)]
            if (window)
                modes.add(window.taskbar_grouping)
        }
        byApp[item.appId] = modes
    }
    for (const app of Object.keys(byApp)) {
        const modes = byApp[app]
        if (modes.size === 1 && !modes.has(0))
            grouped = true
        if (modes.has(0) || modes.size > 1 || (modes.size === 0 && fallback === 0))
            blacklist.push(app)
    }
    return {grouped: grouped, blacklist: Array.from(new Set(blacklist)).sort()}
}

function reverse(entries, windows) {
    const tasks = byTask(windows.filter(window => window.taskbar_eligible))
    const positions = []
    const ranked = []
    entries.forEach((row, index) => {
        const ids = entry(row).windowIds.map(id => tasks[key(id)]).filter(Boolean)
        if (ids.length) {
            positions.push(index)
            ranked.push({index: index, column: Math.min(...ids.map(window => window.layout.pos_in_scrolling_layout[0]))})
        }
    })
    ranked.sort((a, b) => a.column - b.column)
    const result = entries.map((row, index) => index)
    positions.forEach((position, index) => result[position] = ranked[index].index)
    return result
}

function pinned(entries, launchers) {
    const positions = []
    const ranked = []
    entries.forEach((row, index) => {
        const rank = launchers.indexOf(entry(row).launcherUrl)
        if (rank >= 0) {
            positions.push(index)
            ranked.push({index: index, rank: rank})
        }
    })
    ranked.sort((a, b) => a.rank - b.rank)
    const result = entries.map((row, index) => index)
    positions.forEach((position, index) => result[position] = ranked[index].index)
    return result
}

function ready(entries, windows) {
    const ids = new Set()
    for (const row of entries) {
        for (const id of entry(row).windowIds)
            ids.add(key(id))
    }
    return windows.filter(window => window.taskbar_eligible).every(window => ids.has(key(window.task_id)))
}
