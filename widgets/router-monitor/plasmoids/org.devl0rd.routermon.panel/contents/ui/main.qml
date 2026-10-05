import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import "lib"
import "lib/History.js" as History
import "lib/Ring.js" as Ring
import "lib/PopStyle.js" as Style
import "lib/Format.js" as Fmt

RouterCommands {
    id: root

    readonly property bool overlayHost: Plasmoid.pluginName === "org.devl0rd.routermon.overlay"
    readonly property bool overlayVisible: overlayHost && monitorOverlay.targets.length > 0
    Plasmoid.status: overlayHost ? PlasmaCore.Types.HiddenStatus : PlasmaCore.Types.ActiveStatus
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

    RouterData {
        id: routerData
        active: root.overlayHost ? root.overlayVisible : (root.inPanel || root.visible)
        onUpdated: root.onSnapshot()
    }

    readonly property var snap: routerData.snapshot
    readonly property bool ready: routerData.ready
    readonly property bool online: routerData.online
    readonly property bool paused: routerData.paused
    readonly property var info: snap.info || ({})
    readonly property var system: snap.system || ({})
    readonly property var network: snap.network || ({})
    readonly property bool localFallback: snap.fallback === "local"
    readonly property var wifi: snap.wifi || ({})
    readonly property var radios: wifi.radios || []
    readonly property var stations: wifi.stations || []
    readonly property var leases: (snap.clients || {}).leases || []
    readonly property var dns: snap.dns || null
    readonly property var security: snap.security || ({})
    readonly property bool wanUp: network.wan_up === true
    readonly property int onlineCount: leases.filter(lease => lease.connected === true).length

    readonly property bool collectorStale: routerData.stale
    readonly property string routerState: collectorStale ? "stale" : paused ? "paused" : !online ? "offline" : !wanUp && ready ? "wandown" : "ok"
    readonly property color stateColor: routerState === "ok" ? Kirigami.Theme.positiveTextColor
                                      : routerState === "wandown" || routerState === "paused" ? Kirigami.Theme.neutralTextColor
                                      : Kirigami.Theme.negativeTextColor
    readonly property string stateText: routerState === "stale" ? i18n("Collector stopped")
                                      : routerState === "paused" ? i18n("Monitoring paused")
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
    property bool tooltipWanted: false
    function tooltipText() {
        if (routerState === "stale")
            return stateText
        if (localFallback)
            return i18n("Router not connected · showing this computer's network I/O and ping")
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
    toolTipSubText: tooltipWanted ? tooltipText() : ""

    function middleClick() {
        if (Plasmoid.configuration.middleClickPause)
            ctlRun("pause toggle")
    }

    compactRepresentation: CompactView {}
    fullRepresentation: FullView {}

    MonitorOverlay {
        id: monitorOverlay
        active: root.overlayHost
        slot: 2
        content: Component { CompactView {} }
        popupContent: Component { FullView {} }
    }
}
