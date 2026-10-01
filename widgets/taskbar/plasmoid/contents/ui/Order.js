function windowKey(value) {
    return String(value).replace(/[{}]/g, "").toLowerCase()
}

function groups(entries, windows, output) {
    const byTask = {}
    for (const window of windows) {
        if (window.output === output)
            byTask[windowKey(window.task_id)] = window.id
    }
    const seen = {}
    return entries.map(row => {
        const entry = row.entry || row
        const ids = []
        for (const task of entry.windowIds) {
            const id = byTask[windowKey(task)]
            if (id && !seen[id]) {
                seen[id] = true
                ids.push(id)
            }
        }
        return ids
    })
}
