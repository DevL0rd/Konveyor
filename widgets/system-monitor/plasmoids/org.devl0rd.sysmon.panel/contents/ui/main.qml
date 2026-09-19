import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import "lib"
import "lib/History.js" as History
import "lib/PopStyle.js" as Style
import "lib/Format.js" as Fmt

PlasmoidItem {
    id: root

    readonly property string panelIcon: Plasmoid.configuration.panelIcon || "cpu"
    Plasmoid.icon: panelIcon
    Plasmoid.title: i18n("System Monitor")

    readonly property color accent: Plasmoid.configuration.accentColor !== "" ? Plasmoid.configuration.accentColor : Kirigami.Theme.highlightColor

    property var snap: ({})
    readonly property var cpu: snap.cpu || ({})
    readonly property var mem: snap.mem || ({})
    readonly property var gpu: snap.gpu || null
    property bool collectorAlive: false
    property int tick: 0

    readonly property int historyLength: Math.max(60, Math.min(600, Plasmoid.configuration.historyLength))
    property var history: History.make(historyLength)
    onHistoryLengthChanged: history = History.resized(history, historyLength)

    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool dataWanted: inPanel || visible
    onDataWantedChanged: if (!dataWanted) watchdog.stop()
    property bool popupAlive: !inPanel
    preferredRepresentation: inPanel ? compactRepresentation : fullRepresentation
    onExpandedChanged: function() {
        if (root.expanded) {
            releasePopup.stop()
            popupAlive = true
        } else if (inPanel) {
            releasePopup.restart()
        }
    }
    Timer {
        id: releasePopup
        interval: 1500
        onTriggered: root.popupAlive = root.expanded || !root.inPanel
    }


    function cpuShort() {
        return (snap.cpu_model || "").replace(/\(R\)|\(TM\)/g, "").replace(/\d+th Gen /, "").replace("Intel Core ", "").replace(/\s+/g, " ").trim()
    }
    function gpuShort(name) {
        return (name || i18n("GPU")).replace("NVIDIA GeForce ", "").replace("NVIDIA ", "").replace("AMD Radeon ", "Radeon ")
    }

    function metric(key, label, keywords, get, fmt, max, warn, crit) {
        return {
            key: key, label: label, keywords: keywords, get: get, fmt: fmt, max: max,
            crit: crit === undefined ? undefined : crit,
            color: warn === undefined ? undefined : function(v) { return Style.heatStrong(v, warn, crit, Kirigami.Theme) }
        }
    }

    readonly property var cpuMetrics: [
        metric("usage", i18n("Usage"), ["usage", "load", "utilization"], c => c.total || 0, v => Math.round(v) + "%", c => 100, 60, 85),
        metric("temp", i18n("Temp"), ["temp", "temperature", "heat", "hot"], c => c.temp || 0, v => Math.round(v) + "°C", c => 100, 80, 95),
        metric("clock", i18n("Clock"), ["clock", "freq", "frequency", "ghz", "speed"], c => c.freq || 0, v => v.toFixed(2) + " GHz",
               c => Plasmoid.configuration.cpuClockMax > 0 ? Plasmoid.configuration.cpuClockMax : (c.clock_max || 6)),
        metric("power", i18n("Power"), ["power", "watts", "watt", "energy"], c => c.watts || 0, v => Math.round(v) + " W",
               c => Plasmoid.configuration.cpuPowerMax > 0 ? Plasmoid.configuration.cpuPowerMax : (c.power_max || 100)),
        metric("fan", i18n("Fan"), ["fan", "rpm", "cooling"], c => c.fan || 0, v => v > 0 ? Math.round(v) + " RPM" : "—",
               c => Plasmoid.configuration.cpuFanMax > 0 ? Plasmoid.configuration.cpuFanMax : (c.fan_max || 6000))
    ]
    readonly property var gpuMetrics: [
        metric("usage", i18n("Usage"), ["usage", "load", "utilization"], g => g.util || 0, v => Math.round(v) + "%", g => 100, 60, 85),
        metric("temp", i18n("Temp"), ["temp", "temperature", "heat", "hot"], g => g.temp || 0, v => Math.round(v) + "°C", g => 100, 75, 88),
        metric("clock", i18n("Clock"), ["clock", "freq", "frequency", "mhz", "speed"], g => g.clock_gr || 0, v => Math.round(v) + " MHz",
               g => Plasmoid.configuration.gpuClockMax > 0 ? Plasmoid.configuration.gpuClockMax : (g.clock_max || 3000)),
        metric("power", i18n("Power"), ["power", "watts", "watt", "energy"], g => g.power || 0, v => Math.round(v) + " W",
               g => Plasmoid.configuration.gpuPowerMax > 0 ? Plasmoid.configuration.gpuPowerMax : (g.power_max || 175)),
        metric("fan", i18n("Fan"), ["fan", "cooling"], g => g.fan || 0, v => Math.round(v) + "%",
               g => Plasmoid.configuration.gpuFanMax > 0 ? Plasmoid.configuration.gpuFanMax : 100)
    ]

    function record() {
        for (const m of cpuMetrics)
            History.push(history, "cpu." + m.key, m.get(cpu))
        if (gpu) {
            for (const m of gpuMetrics)
                History.push(history, "gpu." + m.key, m.get(gpu))
        }
        History.push(history, "mem.usage", mem.pct || 0)
        tick++
    }
    function series(key) {
        tick
        return History.values(history, key)
    }

    toolTipMainText: snap.host || i18n("System Monitor")
    property bool tooltipWanted: false
    function tooltipText() {
        if (!snap.cpu)
            return i18n("Waiting for the collector")
        const lines = [
            (snap.uptime ? i18n("Up %1", Fmt.duration(snap.uptime)) : ""),
            i18n("CPU %1% · %2 · %3 GHz · %4 W", Math.round(cpu.total || 0), Fmt.temp(cpu.temp), (cpu.freq || 0).toFixed(1), Math.round(cpu.watts || 0))
        ]
        if (gpu)
            lines.push(i18n("GPU %1% · %2 · %3 W · VRAM %4 / %5", Math.round(gpu.util || 0), Fmt.temp(gpu.temp), Math.round(gpu.power || 0),
                            Style.bytes(gpu.vram_used), Style.bytes(gpu.vram_total)))
        lines.push(i18n("RAM %1 / %2", Style.bytes(mem.used), Style.bytes(mem.total))
                   + ((mem.swap_total || 0) > 0 ? i18n(" · Swap %1 / %2", Style.bytes(mem.swap_used), Style.bytes(mem.swap_total)) : ""))
        return lines.filter(line => line !== "").join("\n")
    }
    toolTipSubText: tooltipWanted ? tooltipText() : ""

    property string cachePath: ""
    P5Support.DataSource {
        id: shellRunner
        engine: "executable"
        onNewData: function(source, data) {
            if (source.indexOf("printf") === 0) {
                root.cachePath = (data.stdout || "").trim()
                root.read()
            }
            disconnectSource(source)
        }
    }
    function run(command) {
        shellRunner.connectSource(command)
    }
    function read() {
        if (!cachePath)
            return
        const xhr = new XMLHttpRequest()
        xhr.open("GET", "file://" + cachePath)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE || !xhr.responseText)
                return
            try {
                root.snap = JSON.parse(xhr.responseText)
            } catch (error) {
                return
            }
            root.collectorAlive = true
            watchdog.restart()
            root.record()
        }
        xhr.send()
    }
    FileWatcher {
        path: root.dataWanted ? root.cachePath : ""
        onChanged: root.read()
    }
    Timer {
        id: watchdog
        interval: Math.max(2000, Plasmoid.configuration.updateInterval * 4)
        onTriggered: root.collectorAlive = false
    }
    function applyInterval() {
        run("$HOME/.local/bin/sysmon-collect --set-interval " + (Math.max(500, Plasmoid.configuration.updateInterval) / 1000))
    }
    Connections {
        target: Plasmoid.configuration
        function onUpdateIntervalChanged() { root.applyInterval() }
    }
    Component.onCompleted: {
        run("printf %s \"$XDG_RUNTIME_DIR/Linux-System-Monitor/data.json\"")
        applyInterval()
    }

    function middleClick() {
        if (Plasmoid.configuration.middleClickAction === "systemmonitor")
            run("plasma-systemmonitor")
    }

    compactRepresentation: CompactView {}
    fullRepresentation: FullView {}

    MonitorOverlay {
        active: Plasmoid.pluginName === "org.devl0rd.sysmon.panel"
        slot: 0
        content: Component { CompactView {} }
        popupContent: Component { FullView {} }
    }
}
