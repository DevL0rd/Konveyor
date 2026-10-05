import QtQuick

ListModel {
    id: rows

    function syncFrozen(desired) {
        const want = {}, map = {}
        for (const row of desired) {
            want[row.pid] = true
            map[row.pid] = row
        }
        for (let r = rows.count - 1; r >= 0; --r) {
            if (want[rows.get(r).pid] !== true)
                rows.remove(r)
        }
        const have = {}
        for (let x = 0; x < rows.count; ++x) {
            const current = rows.get(x)
            have[current.pid] = true
            const next = map[current.pid]
            if (next && (current.depth !== next.depth || current.hasChildren !== next.hasChildren || current.expanded !== next.expanded))
                rows.set(x, next)
        }
        for (const row of desired) {
            if (have[row.pid] !== true)
                rows.append(row)
        }
    }
    function syncModel(desired) {
        const want = {}
        for (const row of desired)
            want[row.pid] = true
        for (let r = rows.count - 1; r >= 0; --r) {
            if (want[rows.get(r).pid] !== true)
                rows.remove(r)
        }
        for (let pos = 0; pos < desired.length; ++pos) {
            const row = desired[pos]
            if (pos < rows.count && rows.get(pos).pid === row.pid) {
                const current = rows.get(pos)
                if (current.depth !== row.depth || current.hasChildren !== row.hasChildren || current.expanded !== row.expanded)
                    rows.set(pos, row)
                continue
            }
            let found = -1
            for (let x = pos + 1; x < rows.count; ++x) {
                if (rows.get(x).pid === row.pid) {
                    found = x
                    break
                }
            }
            if (found < 0) {
                rows.insert(pos, row)
            } else {
                rows.move(found, pos, 1)
                const moved = rows.get(pos)
                if (moved.depth !== row.depth || moved.hasChildren !== row.hasChildren || moved.expanded !== row.expanded)
                    rows.set(pos, row)
            }
        }
    }
}
