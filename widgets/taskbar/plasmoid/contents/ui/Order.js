function windowKey(value) {
    return String(value).replace(/[{}]/g, "").toLowerCase()
}

function interleaved(entry, byTask) {
    if (!entry.group)
        return false
    const columns = new Set()
    for (const task of entry.windowIds) {
        const window = byTask[windowKey(task)]
        if (window && window.taskbar_eligible)
            columns.add(window.layout.pos_in_scrolling_layout[0])
    }
    const positions = Array.from(columns)
    return positions.length > 1 && Math.max(...positions) - Math.min(...positions) + 1 > positions.length
}

function groups(entries, windows, output, preserveInterleaved) {
    const byTask = {}
    for (const window of windows) {
        if (window.output === output)
            byTask[windowKey(window.task_id)] = window
    }
    const seen = {}
    return entries.map(row => {
        const entry = row.entry || row
        const ids = []
        if (preserveInterleaved && interleaved(entry, byTask))
            return ids
        for (const task of entry.windowIds) {
            const window = byTask[windowKey(task)]
            const id = window && window.id
            if (id && !seen[id]) {
                seen[id] = true
                ids.push(id)
            }
        }
        return ids
    })
}
