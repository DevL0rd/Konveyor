import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import "lib"
import "lib/History.js" as History
import "lib/Ring.js" as Ring
import "lib/PopStyle.js" as Style
import "lib/Format.js" as Fmt

PlasmoidItem {
    id: root

    readonly property var lockedTabs: ({
        "org.devl0rd.routermon.network": { key: "network", label: i18n("Network") },
        "org.devl0rd.routermon.wifi": { key: "wifi", label: i18n("WiFi") },
        "org.devl0rd.routermon.clients": { key: "clients", label: i18n("Clients") },
        "org.devl0rd.routermon.dns": { key: "dns", label: i18n("DNS") },
        "org.devl0rd.routermon.speedtest": { key: "speed", label: i18n("Speed Test") },
        "org.devl0rd.routermon.system": { key: "system", label: i18n("System") }
    })
    readonly property var locked: lockedTabs[Plasmoid.metaData.pluginId] || null
    readonly property string activeTab: locked ? locked.key : tabKey

    Plasmoid.icon: Plasmoid.metaData.iconName
    Plasmoid.title: locked ? i18n("Router · %1", locked.label) : i18n("Router")

    readonly property color accent: Plasmoid.configuration.accentColor !== "" ? Plasmoid.configuration.accentColor : Kirigami.Theme.highlightColor
    readonly property color downColor: Style.hue("down", Kirigami.Theme)
    readonly property color upColor: Style.hue("up", Kirigami.Theme)
    readonly property string ctl: "$HOME/.local/bin/routermon-ctl"

    RouterData {
        id: routerData
        onUpdated: root.onSnapshot()
    }

    readonly property var snap: routerData.snapshot
    readonly property bool ready: routerData.ready
    readonly property bool online: routerData.online
    readonly property bool paused: routerData.paused
    readonly property var info: snap.info || ({})
    readonly property var system: snap.system || ({})
    readonly property var network: snap.network || ({})
    readonly property var wifi: snap.wifi || ({})
    readonly property var radios: wifi.radios || []
    readonly property var stations: wifi.stations || []
    readonly property var leases: (snap.clients || {}).leases || []
    readonly property var dns: snap.dns || null
    readonly property var security: snap.security || ({})
    readonly property bool wanUp: network.wan_up === true
    readonly property int onlineCount: leases.filter(lease => lease.connected === true).length

    readonly property string routerState: paused ? "paused" : !online ? "offline" : !wanUp && ready ? "wandown" : "ok"
    readonly property color stateColor: routerState === "ok" ? Kirigami.Theme.positiveTextColor
                                      : routerState === "wandown" || routerState === "paused" ? Kirigami.Theme.neutralTextColor
                                      : Kirigami.Theme.negativeTextColor
    readonly property string stateText: routerState === "paused" ? i18n("Monitoring paused")
                                      : routerState === "offline" ? i18n("Router not connected")
                                      : routerState === "wandown" ? i18n("Internet is down") : i18n("Online")

    property var history: History.make(120)
    property var clientRings: ({})
    property int tick: 0

    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical
    property bool popupAlive: !inPanel
    preferredRepresentation: inPanel ? compactRepresentation : fullRepresentation
    onExpandedChanged: function() {
        if (root.expanded) {
            releasePopup.stop()
            popupAlive = true
            syncClients()
        } else if (inPanel) {
            releasePopup.restart()
        }
    }
    Timer {
        id: releasePopup
        interval: 1500
        onTriggered: root.popupAlive = root.expanded || !root.inPanel
    }


    function speed(mbps) {
        const v = mbps || 0
        if (v >= 1000)
            return { value: (v / 1000).toFixed(2), unit: i18n("Gb/s") }
        if (v >= 100)
            return { value: Math.round(v) + "", unit: i18n("Mb/s") }
        if (v >= 1)
            return { value: v.toFixed(1), unit: i18n("Mb/s") }
        return { value: Math.round(v * 1000) + "", unit: i18n("Kb/s") }
    }
    function speedText(mbps) {
        const s = speed(mbps)
        return s.value + " " + s.unit
    }
    function kib(value) {
        return Style.bytes((value || 0) * 1024)
    }
    function nameForMac(mac) {
        const lower = (mac || "").toLowerCase()
        const lease = leases.find(entry => (entry.mac || "").toLowerCase() === lower)
        return lease ? (lease.name || lease.ip) : mac
    }
    function shortBand(band) {
        return (band || "").replace("GHz", "G").replace("-", " · ")
    }

    function onSnapshot() {
        History.push(history, "down", network.down_mbps || 0)
        History.push(history, "up", network.up_mbps || 0)
        History.push(history, "ping", network.ping_rtt || 0)
        History.push(history, "cpu", (system.cpu || {}).total || 0)
        const lan = (network.ifaces || {}).br0 || {}
        History.push(history, "lan.rx", lan.rx_mbps || 0)
        History.push(history, "lan.tx", lan.tx_mbps || 0)

        const rings = {}
        for (const lease of leases) {
            const mac = (lease.mac || "").toLowerCase()
            const ring = clientRings[mac] || Ring.make(40)
            Ring.push(ring, lease.traffic_bps > 0 ? lease.traffic_bps : 0)
            rings[mac] = ring
        }
        clientRings = rings

        const down = network.down_mbps || 0
        const up = network.up_mbps || 0
        if (down > Plasmoid.configuration.peakDown)
            Plasmoid.configuration.peakDown = down
        if (up > Plasmoid.configuration.peakUp)
            Plasmoid.configuration.peakUp = up

        if (popupAlive)
            syncClients()
        tick++
    }
    function series(key) {
        tick
        return History.values(history, key)
    }
    function clientSeries(mac) {
        tick
        const ring = clientRings[(mac || "").toLowerCase()]
        return ring ? Ring.values(ring) : []
    }

    property string message
    property bool messageError: false
    Timer {
        id: messageTimer
        interval: 5000
        onTriggered: root.message = ""
    }
    function flashMessage(text, isError) {
        message = text
        messageError = isError === true
        messageTimer.restart()
    }

    P5Support.DataSource {
        id: ctlRunner
        engine: "executable"
        onNewData: function(source, data) {
            try {
                const result = JSON.parse(data.stdout)
                root.flashMessage(result.msg || "", result.ok === false)
            } catch (error) {
                root.flashMessage((data.stderr || "").trim(), true)
            }
            disconnectSource(source)
        }
    }
    function ctlRun(args) {
        ctlRunner.connectSource(ctl + " " + args)
    }

    P5Support.DataSource {
        id: localRunner
        engine: "executable"
        onNewData: function(source, data) { disconnectSource(source) }
    }
    function launch(command) {
        localRunner.connectSource(command)
    }

    TextEdit { id: clipboard; visible: false }
    function copy(text) {
        clipboard.text = text
        clipboard.selectAll()
        clipboard.copy()
        flashMessage(i18n("Copied %1", text), false)
    }

    function pinnedList() {
        return (Plasmoid.configuration.pinnedMacs || "").split(",").filter(entry => entry)
    }
    function isPinned(mac) {
        return pinnedList().indexOf(mac) >= 0
    }
    function togglePin(mac) {
        const list = pinnedList()
        const at = list.indexOf(mac)
        if (at >= 0)
            list.splice(at, 1)
        else
            list.push(mac)
        Plasmoid.configuration.pinnedMacs = list.join(",")
        syncClients()
    }
    function sshTarget(ip) {
        const user = Plasmoid.configuration.sshUser
        return user ? user + "@" + ip : ip
    }

    signal promptRequested(string mode, string mac, string value)

    property string expandedMac
    ListModel { id: clientModel }
    readonly property alias clients: clientModel

    function clientRow(lease, stationsByMac) {
        const mac = (lease.mac || "").toLowerCase()
        const station = stationsByMac[mac]
        const ring = clientRings[mac]
        return {
            mac: lease.mac, name: lease.name || lease.ip, ip: lease.ip,
            connected: lease.connected === true, blocked: lease.blocked === true,
            traffic: lease.traffic_bps === undefined ? -1 : lease.traffic_bps,
            avgTraffic: ring ? Ring.avg(ring) : 0,
            wireless: station !== undefined, band: station ? station.band : "",
            rssi: station ? station.rssi : 0,
            txMbps: station ? station.tx_mbps || 0 : 0, rxMbps: station ? station.rx_mbps || 0 : 0, phyMbps: station ? station.phy_mbps || 0 : 0,
            pinned: isPinned(lease.mac)
        }
    }
    function syncClients() {
        const stationsByMac = {}
        for (const station of stations)
            stationsByMac[(station.mac || "").toLowerCase()] = station
        const filter = Plasmoid.configuration.clientFilter
        const desired = leases.map(lease => clientRow(lease, stationsByMac)).filter(row => {
            if (filter === "online") return row.connected
            if (filter === "wifi") return row.wireless
            if (filter === "wired") return !row.wireless
            if (filter === "blocked") return row.blocked
            return true
        })
        const by = Plasmoid.configuration.sortBy
        desired.sort((a, b) => {
            if (a.pinned !== b.pinned) return a.pinned ? -1 : 1
            if (by === "traffic") return b.avgTraffic - a.avgTraffic
            if (by === "signal") return (b.wireless ? b.rssi : -999) - (a.wireless ? a.rssi : -999)
            if (by === "ip") {
                const na = a.ip.split(".").map(Number), nb = b.ip.split(".").map(Number)
                for (let k = 0; k < 4; ++k)
                    if (na[k] !== nb[k]) return na[k] - nb[k]
                return 0
            }
            return a.name.toLowerCase() < b.name.toLowerCase() ? -1 : 1
        })
        const macs = desired.map(row => row.mac)
        for (let i = clientModel.count - 1; i >= 0; --i) {
            if (macs.indexOf(clientModel.get(i).mac) < 0)
                clientModel.remove(i)
        }
        for (let pos = 0; pos < desired.length; ++pos) {
            let current = -1
            for (let x = pos; x < clientModel.count; ++x) {
                if (clientModel.get(x).mac === desired[pos].mac) {
                    current = x
                    break
                }
            }
            if (current < 0) {
                clientModel.insert(pos, desired[pos])
            } else {
                if (current !== pos)
                    clientModel.move(current, pos, 1)
                clientModel.set(pos, desired[pos])
            }
        }
    }
    Connections {
        target: Plasmoid.configuration
        function onClientFilterChanged() { root.syncClients() }
        function onSortByChanged() { root.syncClients() }
    }

    readonly property var lastResult: {
        const text = Plasmoid.configuration.lastResult
        if (!text) return null
        try { return JSON.parse(text) } catch (error) { return null }
    }
    readonly property var speedHistory: {
        try { return JSON.parse(Plasmoid.configuration.speedHistory || "[]") } catch (error) { return [] }
    }
    property bool testing: false
    property var live: null
    P5Support.DataSource {
        id: speedRunner
        engine: "executable"
        onNewData: function(source, data) {
            const text = (data.stdout || "").trim()
            if (text) {
                try {
                    const result = JSON.parse(text)
                    Plasmoid.configuration.lastResult = text
                    if (result.ok !== false) {
                        if (result.down_mbps) Plasmoid.configuration.peakDown = result.down_mbps
                        if (result.up_mbps) Plasmoid.configuration.peakUp = result.up_mbps
                        const next = [result].concat(root.speedHistory).slice(0, 20)
                        Plasmoid.configuration.speedHistory = JSON.stringify(next)
                    } else {
                        root.flashMessage(result.error || i18n("Speed test failed"), true)
                    }
                } catch (error) {
                    root.flashMessage(i18n("Speed test failed"), true)
                }
            }
            root.testing = false
            root.live = null
            disconnectSource(source)
        }
    }
    FileWatcher {
        path: root.testing && routerData.cachePath ? routerData.cachePath.replace(/data\.json$/, "speedtest_live.json") : ""
        onChanged: {
            const xhr = new XMLHttpRequest()
            xhr.open("GET", "file://" + path)
            xhr.onreadystatechange = function() {
                if (xhr.readyState !== XMLHttpRequest.DONE || !xhr.responseText || !root.testing)
                    return
                try { root.live = JSON.parse(xhr.responseText) } catch (error) {}
            }
            xhr.send()
        }
    }
    function runSpeedTest() {
        if (testing)
            return
        live = null
        testing = true
        speedRunner.connectSource("$HOME/.local/bin/routermon-speedtest")
    }
    function deleteSpeedResult(index) {
        const next = speedHistory.slice()
        next.splice(index, 1)
        Plasmoid.configuration.speedHistory = JSON.stringify(next)
    }
    function ago(ts) {
        if (!ts) return ""
        const seconds = Math.max(0, Date.now() / 1000 - ts)
        if (seconds < 90) return i18n("just now")
        if (seconds < 3600) return i18np("%1 minute ago", "%1 minutes ago", Math.round(seconds / 60))
        if (seconds < 86400) return i18np("%1 hour ago", "%1 hours ago", Math.round(seconds / 3600))
        return i18np("%1 day ago", "%1 days ago", Math.round(seconds / 86400))
    }

    property string tabKey: Plasmoid.configuration.rememberTab ? Plasmoid.configuration.currentTab : Plasmoid.configuration.defaultTab
    onTabKeyChanged: Plasmoid.configuration.currentTab = tabKey
    Connections {
        target: root
        function onExpandedChanged() {
            if (root.expanded && !Plasmoid.configuration.rememberTab)
                root.tabKey = Plasmoid.configuration.defaultTab
        }
    }

    toolTipMainText: info.model ? i18n("%1 · %2", info.model, network.wan_ip || i18n("no WAN")) : i18n("Router")
    toolTipSubText: {
        if (routerState === "paused" || routerState === "offline")
            return stateText
        const lines = [i18n("↓ %1  ↑ %2 · %3 ms · %4% loss", speedText(network.down_mbps), speedText(network.up_mbps),
                            (network.ping_rtt || 0).toFixed(0), network.ping_loss || 0),
                       i18n("%1 devices · %2 on WiFi", leases.length, stations.length)]
        if (dns)
            lines.push(i18n("AdGuard %1% blocked", dns.blocked_pct))
        if (routerState === "wandown")
            lines.unshift(stateText)
        return lines.join("\n")
    }

    function middleClick() {
        if (Plasmoid.configuration.middleClickPause)
            ctlRun("pause toggle")
    }

    compactRepresentation: CompactView {}
    fullRepresentation: FullView {}
}
