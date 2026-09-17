import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import "../lib"
import "../lib/Highlight.js" as Highlight
import "../lib/Format.js" as Fmt

Item {
    id: results

    property string query
    readonly property string needle: query.toLowerCase()

    function has(text) {
        return Highlight.matches(text, needle)
    }

    readonly property var devices: {
        root.tick
        const stationsByMac = {}
        for (const station of root.stations)
            stationsByMac[(station.mac || "").toLowerCase()] = station
        return root.leases.filter(lease => has(lease.name) || has(lease.ip) || has(lease.mac))
                          .map(lease => root.clientRow(lease, stationsByMac))
    }
    readonly property var radios: root.radios.filter(radio => has(radio.band) || has(radio.ssid) || has("channel " + radio.chan) || has("wifi"))
    readonly property var ports: (root.network.ports || []).filter(port => has(port.port) || has("port") || has("wired"))
    readonly property var domains: {
        if (!root.dns) return []
        const found = []
        const lists = [
            { entries: root.dns.top_blocked || [], kind: i18n("Blocked") },
            { entries: root.dns.top_queried || [], kind: i18n("Queried") },
            { entries: root.dns.top_clients || [], kind: i18n("DNS client") }
        ]
        for (const list of lists) {
            for (const entry of list.entries) {
                if (has(entry.name))
                    found.push({ name: entry.name, count: entry.count, kind: list.kind })
            }
        }
        return found
    }
    readonly property var actions: [
        { keywords: ["reboot", "restart router", "reset"], text: i18n("Reboot router"), icon: "system-reboot", confirm: true, run: () => root.ctlRun("reboot"), visible: Plasmoid.configuration.showReboot },
        { keywords: ["restart wifi", "wifi restart", "wireless"], text: i18n("Restart WiFi"), icon: "network-wireless", confirm: true, run: () => root.ctlRun("restart-wifi"), visible: Plasmoid.configuration.showRestartWifi },
        { keywords: ["pause", "resume", "stop monitoring", "monitoring"], text: root.paused ? i18n("Resume monitoring") : i18n("Pause monitoring"), icon: root.paused ? "media-playback-start" : "media-playback-pause", run: () => root.ctlRun("pause toggle"), visible: true },
        { keywords: ["protection", "adguard", "adblock", "block ads"], text: root.dns && root.dns.protection ? i18n("Turn AdGuard protection off") : i18n("Turn AdGuard protection on"), icon: "security-high", run: () => root.ctlRun("protection " + (root.dns && root.dns.protection ? "off" : "on")), visible: root.dns !== null },
        { keywords: ["speed test", "speedtest", "test speed", "bandwidth"], text: i18n("Run speed test"), icon: "speedometer", run: () => root.runSpeedTest(), visible: !root.testing },
        { keywords: ["web ui", "admin", "router page", "settings"], text: i18n("Open router web UI"), icon: "internet-web-browser", run: () => root.launch("xdg-open " + root.info.admin_url), visible: !!root.info.admin_url },
        { keywords: ["adguard", "dns page"], text: i18n("Open AdGuard Home"), icon: "security-high", run: () => root.launch("xdg-open " + root.info.agh_url), visible: !!root.info.agh_url }
    ].filter(action => action.visible && action.keywords.some(word => word.indexOf(needle) >= 0 || (needle.length > 3 && needle.indexOf(word) >= 0)))

    readonly property int count: devices.length + radios.length + ports.length + domains.length + actions.length

    function activateFirst() {
        if (devices.length > 0) {
            root.expandedMac = devices[0].mac
        } else if (actions.length > 0 && !actions[0].confirm) {
            actions[0].run()
        } else if (radios.length > 0) {
            root.tabKey = "wifi"
        } else if (domains.length > 0) {
            root.tabKey = "dns"
        }
    }

    component SectionHeader: PlasmaComponents.Label {
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.smallSpacing
        font.pointSize: Kirigami.Theme.smallFont.pointSize
        font.weight: Font.DemiBold
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 0.6
        opacity: 0.6
    }

    component ResultRow: MouseArea {
        id: resultRow
        property string icon
        property string title
        property string detail
        property string trailing
        Layout.fillWidth: true
        implicitHeight: resultLayout.implicitHeight + Kirigami.Units.smallSpacing * 3
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        Rectangle {
            anchors.fill: parent
            radius: Kirigami.Units.cornerRadius * 2
            color: Qt.alpha(Kirigami.Theme.textColor, resultRow.containsMouse ? 0.075 : 0.045)
            border.width: 1
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.07)
        }
        RowLayout {
            id: resultLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: Kirigami.Units.smallSpacing * 2
            spacing: Kirigami.Units.smallSpacing * 1.5
            Kirigami.Icon {
                source: resultRow.icon
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: Kirigami.Units.iconSizes.small
                opacity: 0.75
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                PlasmaComponents.Label {
                    text: Highlight.mark(resultRow.title, results.query, Kirigami.Theme.highlightColor)
                    textFormat: Text.StyledText
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                PlasmaComponents.Label {
                    visible: resultRow.detail !== ""
                    text: Highlight.mark(resultRow.detail, results.query, Kirigami.Theme.highlightColor)
                    textFormat: Text.StyledText
                    font: Kirigami.Theme.smallFont
                    opacity: 0.65
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
            PlasmaComponents.Label {
                visible: resultRow.trailing !== ""
                text: resultRow.trailing
                font.features: { "tnum": 1 }
                opacity: 0.75
            }
            Kirigami.Icon {
                source: "go-next"
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: Kirigami.Units.iconSizes.small
                opacity: 0.4
            }
        }
    }

    PlasmaExtras.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: results.count === 0
        iconName: "edit-find"
        text: i18n("Nothing matches “%1”", results.query)
        explanation: i18n("Search by device name, IP or MAC, WiFi band or network name, a domain, or an action like reboot or speed test")
    }

    PopScroll {
        anchors.fill: parent
        visible: results.count > 0
        Component.onCompleted: column.spacing = Kirigami.Units.smallSpacing * 1.5

        SectionHeader {
            visible: results.actions.length > 0
            text: i18n("Actions")
        }
        Repeater {
            model: results.actions
            Loader {
                required property var modelData
                Layout.fillWidth: true
                sourceComponent: modelData.confirm ? confirmAction : plainAction
                Component {
                    id: confirmAction
                    RowLayout {
                        PopConfirm {
                            label: modelData.text
                            iconName: modelData.icon
                            onConfirmed: modelData.run()
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
                Component {
                    id: plainAction
                    RowLayout {
                        PlasmaComponents.Button {
                            text: modelData.text
                            icon.name: modelData.icon
                            onClicked: modelData.run()
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }
        }

        SectionHeader {
            visible: results.devices.length > 0
            text: i18np("%1 device", "%1 devices", results.devices.length)
        }
        Repeater {
            model: results.devices
            ClientRow {
                required property var modelData
                Layout.fillWidth: true
                mac: modelData.mac
                name: modelData.name
                ip: modelData.ip
                connected: modelData.connected
                blocked: modelData.blocked
                traffic: modelData.traffic
                wireless: modelData.wireless
                band: modelData.band
                rssi: modelData.rssi
                txMbps: modelData.txMbps
                rxMbps: modelData.rxMbps
                phyMbps: modelData.phyMbps
                pinned: modelData.pinned
                query: results.query
            }
        }

        SectionHeader {
            visible: results.radios.length > 0
            text: i18n("WiFi radios")
        }
        Repeater {
            model: results.radios
            ResultRow {
                required property var modelData
                icon: "network-wireless"
                title: modelData.band
                detail: (modelData.ssid || "") + " · " + i18n("channel %1", modelData.chan)
                trailing: i18np("%1 client", "%1 clients", modelData.clients || 0)
                onClicked: root.tabKey = "wifi"
            }
        }

        SectionHeader {
            visible: results.ports.length > 0
            text: i18n("Wired ports")
        }
        Repeater {
            model: results.ports
            ResultRow {
                required property var modelData
                icon: "network-wired"
                title: modelData.port
                detail: modelData.link || ""
                onClicked: root.tabKey = "network"
            }
        }

        SectionHeader {
            visible: results.domains.length > 0
            text: i18n("DNS")
        }
        Repeater {
            model: results.domains
            ResultRow {
                required property var modelData
                icon: modelData.kind === i18n("Blocked") ? "dialog-cancel" : "internet-web-browser"
                title: modelData.name
                detail: modelData.kind
                trailing: (modelData.count || 0).toLocaleString(Qt.locale(), "f", 0)
                onClicked: root.tabKey = "dns"
            }
        }
    }
}
