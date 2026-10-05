import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import "lib"
import "Journal.js" as Journal

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
    readonly property alias warningCount: summary.warningCount
    readonly property alias errorCount: summary.errorCount
    readonly property alias topSources: summary.sources
    readonly property alias activity: summary.activity
    readonly property alias activityAlerts: summary.activityAlerts
    readonly property alias linesPerMinute: summary.linesPerMinute
    readonly property int activityBuckets: 40
    readonly property int activityBucketSeconds: 15

    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool watching: root.expanded || !root.inPanel
    readonly property bool dataWanted: inPanel || visible
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
            summary.tally = null
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
    readonly property string collectorError: logData.error
    readonly property string offlineText: collectorError !== "" ? collectorError : i18n("Collector not running")
    readonly property string stateKey: !logData.online && !root.searchMode ? "offline" : root.paused ? "paused" : "live"
    readonly property color stateColor: stateKey === "live" ? Kirigami.Theme.positiveTextColor
                                      : stateKey === "paused" ? Kirigami.Theme.neutralTextColor
                                      : Kirigami.Theme.negativeTextColor

    Plasmoid.title: i18n("System Log")
    Plasmoid.icon: "utilities-log-viewer"
    toolTipMainText: i18n("System Log")
    toolTipSubText: {
        if (!logData.online)
            return root.offlineText
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
        active: root.dataWanted
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
        let counted = lines.length - 1
        while (counted >= 0 && lines[counted].t !== root.countedT)
            counted--
        for (let i = lines.length - 1; i > counted && (counted >= 0 || lines[i].t > root.countedT); i--) {
            const r = lines[i]
            if (isMuted(r))
                continue
            if (r.p <= 3)
                errors++
            else if (r.p === 4)
                warnings++
        }
        root.countedT = lines[lines.length - 1].t
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
    LogSummary {
        id: summary
        levelMax: root.levelMax
        isMuted: r => root.isMuted(r)
        buckets: root.activityBuckets
        bucketSeconds: root.activityBucketSeconds
    }
    function refreshSummary() { summary.refresh(logData.lines) }

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
                    if (root.shownCursors[rec.cursor])
                        continue
                    root.shownCursors[rec.cursor] = true
                    if (!root.isMuted(rec)) {
                        logModel.append(root.rowFor(rec))
                        added++
                    }
                }
                root.rowsAppended(added)
            }
            const over = logModel.count - Plasmoid.configuration.searchLimit
            if (over > 0)
                logModel.remove(0, over)
        }
    }

    function escapeRegex(s) { return String(s).replace(/[.*+?^${}()|[\]\\]/g, "\\$&") }

    readonly property string journalJson: "journalctl -o json --all --output-fields=MESSAGE,PRIORITY,SYSLOG_IDENTIFIER,_COMM,_SYSTEMD_UNIT,UNIT,_TRANSPORT,_PID,SYSLOG_PID --no-pager"

    property int searchGeneration: 0
    property var shownCursors: ({})
    readonly property string cursorPrefix: "\"$XDG_RUNTIME_DIR\"/konveyor-logmon-search-" + Plasmoid.id + "-"

    function afterCursor(part) {
        const file = root.cursorPrefix + root.searchGeneration + "-" + part + ".cursor"
        return " $(test -e " + file + " || printf -- '-n %s' " + Plasmoid.configuration.searchLimit + ") --cursor-file=" + file
    }

    function runSearch(reset) {
        if (!root.searchMode || !root.popupAlive)
            return
        if (reset) {
            root.searchGeneration++
            root.shownCursors = {}
        }
        const lv = root.levelMax[root.level]
        const prio = lv < 7 ? " -p " + lv : ""
        let cmd
        if (root.search === "") {
            cmd = journalJson + prio + afterCursor("level")
        } else {
            const reArg = Journal.shq(escapeRegex(root.search))
            const fxArg = Journal.shq(root.search)
            cmd = "TIDS=$(journalctl -F SYSLOG_IDENTIFIER --no-pager 2>/dev/null"
                + " | grep -iF -- " + fxArg + " | sed 's/.*/-t &/' | tr '\\n' ' '); "
                + "{ " + journalJson + " --grep " + reArg + prio + afterCursor("grep") + " 2>/dev/null; "
                + "[ -n \"$TIDS\" ] && " + journalJson + " $TIDS" + prio + afterCursor("ids") + " 2>/dev/null; }"
        }
        if (reset) {
            root.querying = true
            cmd += "; find \"$XDG_RUNTIME_DIR\" -maxdepth 1 -name 'konveyor-logmon-search-" + Plasmoid.id + "-*'"
                + " ! -name 'konveyor-logmon-search-" + Plasmoid.id + "-" + root.searchGeneration + "-*' -delete"
        }
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

    function parseRec(line) { return Journal.parseRec(line) }

    signal rowsAppended(int added)
    signal rowRevealRequested(int index)
    function ingest() {
        if (!root.popupAlive)
            return
        const a = logData.lines
        if (a.length === 0)
            return
        const floor = root.lastT - 5000000
        let start = a.length
        while (start > 0 && a[start - 1].t > floor)
            start--
        let added = 0
        for (let i = start; i < a.length; i++) {
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
        if (root.mutedList.length === 0)
            return false
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
        function onMutedAppsChanged() { root.refreshMuted(); summary.tally = null; root.refreshSummary(); root.applyMode() }
    }

    signal lineCopied()
    signal searchRequested(string text)
    function lineText(m) {
        return m.time + "  " + m.app + (m.pid ? "[" + m.pid + "]" : "") + ": " + m.msg
    }
    LogActions {
        id: logActions
        model: logModel
        clipboard: clip
        lineText: m => root.lineText(m)
        onCopied: root.lineCopied()
    }
    TextEdit { id: clip; visible: false }
    function copyText(text) { logActions.copyText(text) }
    function copyAll() { logActions.copyAll() }
    function askClaude(time, app, pid, msg, prio) { logActions.askClaude(time, app, pid, msg, prio) }

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
