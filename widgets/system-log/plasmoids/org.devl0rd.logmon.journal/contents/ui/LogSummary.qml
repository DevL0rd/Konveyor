import QtQuick

Item {
    id: summary

    required property var levelMax
    required property var isMuted
    required property int buckets
    required property int bucketSeconds
    property var tally: null
    property int warningCount: 0
    property int errorCount: 0
    property var activity: []
    property var activityAlerts: []
    property real linesPerMinute: 0
    ListModel { id: sourceModel }
    readonly property alias sources: sourceModel

    function tallyLine(t, r, sign) {
        if (summary.isMuted(r))
            return
        if (r.p <= summary.levelMax[2]) t.warnings += sign
        if (r.p <= summary.levelMax[3]) t.errors += sign
        const n = (t.byApp[r.id] || 0) + sign
        if (n > 0)
            t.byApp[r.id] = n
        else
            delete t.byApp[r.id]
    }
    function updateTally(lines) {
        const t = summary.tally
        const prev = t ? t.lines : null
        let added = 0
        if (prev && prev.length > 0 && lines.length > 0) {
            const last = prev[prev.length - 1]
            let j = lines.length - 1
            while (j >= 0 && !(lines[j].t === last.t && lines[j].pid === last.pid && lines[j].id === last.id && lines[j].m === last.m))
                j--
            added = lines.length - 1 - j
            const removed = prev.length + added - lines.length
            if (j >= 0 && removed >= 0 && removed <= prev.length) {
                for (let i = 0; i < removed; i++)
                    tallyLine(t, prev[i], -1)
                for (let i = lines.length - added; i < lines.length; i++)
                    tallyLine(t, lines[i], 1)
                t.lines = lines
                return t
            }
        }
        const fresh = { lines: lines, warnings: 0, errors: 0, byApp: {} }
        for (let i = 0; i < lines.length; i++)
            tallyLine(fresh, lines[i], 1)
        summary.tally = fresh
        return fresh
    }
    function refresh(lines) {
        const t = updateTally(lines)
        const byApp = t.byApp
        summary.warningCount = t.warnings
        summary.errorCount = t.errors
        const total = new Array(summary.buckets).fill(0)
        const alerts = new Array(summary.buckets).fill(0)
        const nowMicros = Date.now() * 1000
        const bucketMicros = summary.bucketSeconds * 1000000
        let recent = 0
        for (let i = lines.length - 1; i >= 0; i--) {
            const r = lines[i]
            const back = Math.floor((nowMicros - r.t) / bucketMicros)
            if (back >= summary.buckets)
                break
            if (back < 0 || summary.isMuted(r))
                continue
            total[summary.buckets - 1 - back]++
            if (r.p <= 4)
                alerts[summary.buckets - 1 - back]++
            if (back < 4)
                recent++
        }
        if (!sameValues(summary.activity, total))
            summary.activity = total
        if (!sameValues(summary.activityAlerts, alerts))
            summary.activityAlerts = alerts
        summary.linesPerMinute = recent
        const top = Object.keys(byApp)
            .map(app => ({ app: app, hits: byApp[app] }))
            .sort((a, b) => b.hits - a.hits)
            .slice(0, 6)
        for (let i = 0; i < top.length; i++) {
            if (i < sourceModel.count) {
                if (sourceModel.get(i).app !== top[i].app)
                    sourceModel.setProperty(i, "app", top[i].app)
                if (sourceModel.get(i).hits !== top[i].hits)
                    sourceModel.setProperty(i, "hits", top[i].hits)
            } else {
                sourceModel.append(top[i])
            }
        }
        if (sourceModel.count > top.length)
            sourceModel.remove(top.length, sourceModel.count - top.length)
    }
    function sameValues(a, b) {
        if (a.length !== b.length)
            return false
        for (let i = 0; i < a.length; i++)
            if (a[i] !== b[i]) return false
        return true
    }
}
