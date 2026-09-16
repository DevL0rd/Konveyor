import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import "lib"

PlasmoidItem {
    id: root

    readonly property color accent: Plasmoid.configuration.accentColor !== ""
        ? Plasmoid.configuration.accentColor : Kirigami.Theme.highlightColor

    readonly property var levelMax: [7, 6, 4, 3]
    readonly property var priorityNames: [i18n("Emergency"), i18n("Alert"), i18n("Critical"), i18n("Error"),
                                          i18n("Warning"), i18n("Notice"), i18n("Info"), i18n("Debug")]
    property int level: Plasmoid.configuration.defaultLevel
    property string search: ""
    readonly property bool searchMode: search !== "" || level !== 0
    property bool paused: false
    property bool querying: false
    property double lastT: 0
    property bool atBottom: true
    property bool farFromEnd: false
    property bool hasNew: false

    property int newErrors: 0
    property int newWarnings: 0
    property double countedT: parseFloat(Plasmoid.configuration.lastSeen || "0") || 0
    property var levelCounts: [0, 0, 0, 0]
    property var topSources: []
    property var activity: []
    property var activityAlerts: []
    property real linesPerMinute: 0
    readonly property int activityBuckets: 40
    readonly property int activityBucketSeconds: 15

    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool watching: root.expanded || !root.inPanel
    property bool popupAlive: !inPanel
    preferredRepresentation: inPanel ? compactRepresentation : fullRepresentation

    onExpandedChanged: {
        if (root.expanded) {
            releasePopup.stop()
            root.markSeen()
            popupAlive = true
        } else if (inPanel) {
            root.markSeen()
            Plasmoid.configuration.lastSeen = String(root.countedT)
            releasePopup.restart()
        }
    }
    onPopupAliveChanged: {
        if (popupAlive) {
            refreshSummary()
            applyMode()
        } else {
            search = ""
            logModel.clear()
            lastT = 0
        }
    }
    Timer {
        id: releasePopup
        interval: 1500
        onTriggered: root.popupAlive = root.expanded || !root.inPanel
    }

    readonly property bool collectorOnline: logData.online
    readonly property string stateKey: !logData.online && !root.searchMode ? "offline" : root.paused ? "paused" : "live"
    readonly property color stateColor: stateKey === "live" ? Kirigami.Theme.positiveTextColor
                                      : stateKey === "paused" ? Kirigami.Theme.neutralTextColor
                                      : Kirigami.Theme.negativeTextColor

    Plasmoid.title: i18n("System Log")
    Plasmoid.icon: "utilities-log-viewer"
    toolTipMainText: i18n("System Log")
    toolTipSubText: {
        if (!logData.online)
            return i18n("Collector not running")
        if (root.newErrors === 0 && root.newWarnings === 0)
            return i18n("No new errors or warnings since you last looked")
        return i18n("Since you last looked: %1, %2",
                    i18np("%1 error", "%1 errors", root.newErrors),
                    i18np("%1 warning", "%1 warnings", root.newWarnings))
    }

    LogData {
        id: logData
        interval: Plasmoid.configuration.pollInterval
        paused: root.paused
        onUpdated: {
            root.countNew()
            if (!root.popupAlive)
                return
            summaryTimer.running || summaryTimer.start()
            if (root.searchMode)
                root.scheduleSearchRefresh()
            else
                root.ingest()
        }
    }

    function countNew() {
        const lines = logData.lines
        if (lines.length === 0)
            return
        let errors = 0
        let warnings = 0
        for (let i = lines.length - 1; i >= 0 && lines[i].t > root.countedT; i--) {
            const r = lines[i]
            if (isMuted(r))
                continue
            if (r.p <= 3)
                errors++
            else if (r.p === 4)
                warnings++
        }
        root.countedT = Math.max(root.countedT, lines[lines.length - 1].t)
        if (root.watching)
            return
        if (errors)
            root.newErrors += errors
        if (warnings)
            root.newWarnings += warnings
    }
    function markSeen() {
        const lines = logData.lines
        if (lines.length)
            root.countedT = Math.max(root.countedT, lines[lines.length - 1].t)
        root.newErrors = 0
        root.newWarnings = 0
    }

    Timer {
        id: summaryTimer
        interval: 1500
        onTriggered: root.refreshSummary()
    }
    function refreshSummary() {
        const lines = logData.lines
        const counts = [0, 0, 0, 0]
        const byApp = {}
        for (let i = 0; i < lines.length; i++) {
            const r = lines[i]
            if (isMuted(r))
                continue
            for (let k = 0; k < 4; k++)
                if (r.p <= levelMax[k]) counts[k]++
            byApp[r.id] = (byApp[r.id] || 0) + 1
        }
        root.levelCounts = counts
        const total = new Array(activityBuckets).fill(0)
        const alerts = new Array(activityBuckets).fill(0)
        const nowMicros = Date.now() * 1000
        const bucketMicros = activityBucketSeconds * 1000000
        let recent = 0
        for (let i = lines.length - 1; i >= 0; i--) {
            const r = lines[i]
            const back = Math.floor((nowMicros - r.t) / bucketMicros)
            if (back >= activityBuckets)
                break
            if (back < 0 || isMuted(r))
                continue
            total[activityBuckets - 1 - back]++
            if (r.p <= 4)
                alerts[activityBuckets - 1 - back]++
            if (back < 4)
                recent++
        }
        root.activity = total
        root.activityAlerts = alerts
        root.linesPerMinute = recent
        root.topSources = Object.keys(byApp)
            .map(app => ({ app: app, count: byApp[app] }))
            .sort((a, b) => b.count - a.count)
            .slice(0, 6)
    }

    P5Support.DataSource {
        id: journalQuery
        engine: "executable"
        onNewData: function(source, d) {
            disconnectSource(source)
            root.querying = false
            if (!root.searchMode || !root.popupAlive)
                return
            const out = (d.stdout || "").trim()
            if (out !== "") {
                const recs = out.split("\n").map(line => root.parseRec(line)).filter(r => r)
                recs.sort((a, b) => a.t - b.t)
                let added = 0
                for (const rec of recs) {
                    if (rec.t > root.lastT) {
                        root.lastT = rec.t
                        if (!root.isMuted(rec)) {
                            logModel.append(root.rowFor(rec))
                            added++
                        }
                    }
                }
                root.rowsAppended(added)
            }
            const over = logModel.count - Plasmoid.configuration.searchLimit
            if (over > 0)
                logModel.remove(0, over)
        }
    }

    function shq(s) { return "'" + String(s).replace(/'/g, "'\\''") + "'" }
    function escapeRegex(s) { return String(s).replace(/[.*+?^${}()|[\]\\]/g, "\\$&") }

    readonly property string journalJson: "journalctl -o json --all --output-fields=MESSAGE,PRIORITY,SYSLOG_IDENTIFIER,_COMM,_SYSTEMD_UNIT,UNIT,_TRANSPORT,_PID,SYSLOG_PID --no-pager"

    function runSearch(reset) {
        if (!root.searchMode || !root.popupAlive)
            return
        const lv = root.levelMax[root.level]
        const prio = lv < 7 ? " -p " + lv : ""
        const range = (reset || root.lastT === 0)
            ? " -n " + Plasmoid.configuration.searchLimit
            : " --since @" + Math.floor(root.lastT / 1000000)
        let cmd
        if (root.search === "") {
            cmd = journalJson + prio + range
        } else {
            const reArg = shq(escapeRegex(root.search))
            const fxArg = shq(root.search)
            cmd = "TIDS=$(journalctl -F SYSLOG_IDENTIFIER --no-pager 2>/dev/null"
                + " | grep -iF -- " + fxArg + " | sed 's/.*/-t &/' | tr '\\n' ' '); "
                + "{ " + journalJson + " --grep " + reArg + prio + range + " 2>/dev/null; "
                + "[ -n \"$TIDS\" ] && " + journalJson + " $TIDS" + prio + range + " 2>/dev/null; }"
        }
        if (reset)
            root.querying = true
        journalQuery.connectSource(cmd)
    }
    Timer { id: searchDebounce; interval: 300; onTriggered: root.applyMode() }
    Timer { id: searchRefresh; interval: Math.max(250, Plasmoid.configuration.pollInterval); onTriggered: root.runSearch(false) }
    function scheduleSearchRefresh() {
        if (root.searchMode && !root.paused && !searchRefresh.running) searchRefresh.start()
    }

    function applyMode() {
        if (!root.popupAlive)
            return
        logModel.clear()
        root.lastT = 0
        root.hasNew = false
        if (root.searchMode)
            runSearch(true)
        else
            ingest()
    }
    onSearchChanged: searchDebounce.restart()
    onLevelChanged: (search !== "" || level !== 0) ? searchDebounce.restart() : rebuild()

    function matches(r) {
        if (r.p > root.levelMax[root.level])
            return false
        if (root.search === "")
            return true
        const q = root.search.toLowerCase()
        return r.m.toLowerCase().indexOf(q) >= 0 || r.id.toLowerCase().indexOf(q) >= 0
    }

    function rowFor(r) {
        const d = new Date(r.t / 1000)
        const pad = n => ("0" + n).slice(-2)
        return { key: r.t, time: pad(d.getHours()) + ":" + pad(d.getMinutes()) + ":" + pad(d.getSeconds()),
                 date: d.toLocaleDateString(Qt.locale(), Locale.ShortFormat),
                 app: r.id, unit: r.u || "", pid: r.pid, msg: r.m.replace(/\x1b\[[0-9;?]*[ -\/]*[@-~]/g, ""), prio: r.p, expanded: false }
    }

    function parseRec(line) {
        let j
        try { j = JSON.parse(line) } catch (e) { return null }
        let m = j.MESSAGE
        if (Array.isArray(m))
            m = m.map(c => String.fromCharCode(c)).join("")
        const unit = j._SYSTEMD_UNIT || j.UNIT || ""
        let id = j.SYSLOG_IDENTIFIER || j._COMM
        if (!id)
            id = (j._TRANSPORT === "kernel") ? "kernel"
               : (unit.indexOf(".service") >= 0 ? unit.replace(".service", "") : (unit || "?"))
        return { t: parseInt(j.__REALTIME_TIMESTAMP || 0),
                 p: parseInt(j.PRIORITY !== undefined ? j.PRIORITY : 6),
                 id: String(id).slice(0, 40),
                 u: unit, pid: String(j._PID || j.SYSLOG_PID || ""), m: String(m || "") }
    }

    signal rowsAppended(int added)
    signal rowRevealRequested(int index)
    function ingest() {
        if (!root.popupAlive)
            return
        const a = logData.lines
        if (a.length === 0)
            return
        let added = 0
        for (let i = 0; i < a.length; i++) {
            const r = a[i]
            if (r.t > root.lastT && matches(r) && !isMuted(r)) {
                logModel.append(rowFor(r))
                added++
            }
        }
        root.lastT = a[a.length - 1].t
        const over = logModel.count - Plasmoid.configuration.maxRows
        if (over > 0)
            logModel.remove(0, over)
        root.rowsAppended(added)
    }

    function rebuild() {
        if (!root.popupAlive)
            return
        logModel.clear()
        root.lastT = 0
        ingest()
    }

    function clearLog() {
        logModel.clear()
        const a = logData.lines
        if (!root.searchMode && a.length)
            root.lastT = a[a.length - 1].t
        root.hasNew = false
    }

    function toggleExpand(index) {
        if (index < 0 || index >= logModel.count)
            return
        const open = !logModel.get(index).expanded
        logModel.setProperty(index, "expanded", open)
        if (open)
            root.rowRevealRequested(index)
    }

    property var mutedList: []
    function refreshMuted() {
        root.mutedList = (Plasmoid.configuration.mutedApps || "").split(",")
            .map(s => s.trim()).filter(s => s !== "")
    }
    function isMuted(r) {
        const id = r.id.toLowerCase()
        for (let i = 0; i < root.mutedList.length; i++)
            if (id.indexOf(root.mutedList[i].toLowerCase()) >= 0) return true
        return false
    }
    function setMuted(list) {
        Plasmoid.configuration.mutedApps = list.join(", ")
        Plasmoid.configuration.writeConfig()
    }
    function muteApp(app) {
        const list = root.mutedList.slice()
        if (list.indexOf(app) < 0) list.push(app)
        setMuted(list)
    }
    function unmuteApp(app) {
        setMuted(root.mutedList.filter(s => s !== app))
    }
    Connections {
        target: Plasmoid.configuration
        function onMutedAppsChanged() { root.refreshMuted(); root.refreshSummary(); root.applyMode() }
    }

    signal lineCopied()
    signal searchRequested(string text)
    function copyText(text) {
        clip.text = text
        clip.selectAll()
        clip.copy()
        root.lineCopied()
    }
    function lineText(m) {
        return m.time + "  " + m.app + (m.pid ? "[" + m.pid + "]" : "") + ": " + m.msg
    }
    function copyAll() {
        const out = []
        for (let i = 0; i < logModel.count; i++)
            out.push(lineText(logModel.get(i)))
        copyText(out.join("\n"))
    }
    function askClaude(time, app, pid, msg, prio) {
        const ctx = "I saw this entry in my systemd journal (journalctl):\n"
                  + "time: " + time + "\napp: " + app + (pid ? " (pid " + pid + ")" : "")
                  + "\npriority: " + prio + "\nmessage: " + msg
                  + "\n\nWhat does it mean, and is there anything I should do about it?"
        launcher.connectSource("konsole --workdir \"$HOME\" -e claude " + shq(ctx))
    }
    TextEdit { id: clip; visible: false }
    P5Support.DataSource {
        id: launcher
        engine: "executable"
        onNewData: function(source, d) { disconnectSource(source) }
    }

    function middleClick() {
        if (Plasmoid.configuration.middleClickPause)
            root.paused = !root.paused
    }

    ListModel { id: logModel }
    readonly property alias rows: logModel

    Component.onCompleted: {
        refreshMuted()
        if (root.popupAlive) {
            refreshSummary()
            applyMode()
        }
    }

    function prioColor(p) {
        if (p <= 3) return Kirigami.Theme.negativeTextColor
        if (p === 4) return Kirigami.Theme.neutralTextColor
        return Kirigami.Theme.textColor
    }
    function stripeColor(p) {
        if (p <= 3) return Kirigami.Theme.negativeTextColor
        if (p === 4) return Kirigami.Theme.neutralTextColor
        if (p === 5) return Kirigami.Theme.highlightColor
        return Qt.alpha(Kirigami.Theme.textColor, p === 7 ? 0.08 : 0.18)
    }

    compactRepresentation: CompactView {}
    fullRepresentation: FullView {}
}
