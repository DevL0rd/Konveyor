import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import "../lib/PopStyle.js" as Style

PopScroll {
    id: overview

    readonly property var down: root.speed(root.network.down_mbps)
    readonly property var up: root.speed(root.network.up_mbps)

    PopCard {
        title: i18n("Internet")
        icon: "network-wired-activated"
        trailing: root.wanUp ? i18n("Connected") : i18n("Down")
        trailingColor: root.wanUp ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing * 2
            PopStat {
                label: i18n("↓ Download")
                value: overview.down.value
                unit: overview.down.unit
                color: root.downColor
                scale: 1.9
            }
            PopStat {
                label: i18n("↑ Upload")
                value: overview.up.value
                unit: overview.up.unit
                color: root.upColor
                scale: 1.9
            }
            Item { Layout.fillWidth: true }
        }
        Sparkline {
            visible: Plasmoid.configuration.showCharts
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 4
            values: root.series("down")
            values2: root.series("up")
            lineColor: root.downColor
            lineColor2: root.upColor
            gradient: false
            rangeMax: Plasmoid.configuration.maxMbps
            rangeFloor: 1
            tipText: v => root.speedText(v)
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 3
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: Kirigami.Units.largeSpacing

        Tile {
            label: i18n("Ping")
            value: (root.network.ping_rtt || 0).toFixed(0)
            unit: "ms"
            valueColor: Style.heatStrong(root.network.ping_rtt || 0, 40, 100, Kirigami.Theme)
            caption: i18n("to the internet")
            icon: "network-connect"
            onClicked: root.tabKey = "network"
        }
        Tile {
            label: i18n("Loss")
            value: (root.network.ping_loss || 0) + ""
            unit: "%"
            valueColor: Style.heatStrong(root.network.ping_loss || 0, 1, 5, Kirigami.Theme)
            caption: i18n("packets dropped")
            icon: "dialog-warning"
            onClicked: root.tabKey = "network"
        }
        Tile {
            label: i18n("Devices")
            value: root.onlineCount + ""
            unit: "/ " + root.leases.length
            caption: i18np("%1 on WiFi", "%1 on WiFi", root.stations.length)
            icon: "network-server"
            onClicked: root.tabKey = "clients"
        }
        Tile {
            label: i18n("Blocked")
            value: root.dns ? Math.round(root.dns.blocked_pct || 0) + "" : "—"
            unit: root.dns ? "%" : ""
            caption: root.dns ? i18n("of DNS queries") : i18n("AdGuard unavailable")
            icon: "security-high"
            onClicked: root.tabKey = "dns"
        }
        Tile {
            label: i18n("Router CPU")
            value: Math.round((root.system.cpu || {}).total || 0) + ""
            unit: "%"
            valueColor: Style.heatStrong((root.system.cpu || {}).total || 0, 60, 85, Kirigami.Theme)
            caption: i18n("%1 °C", Math.round(root.system.cpu_temp || 0))
            icon: "cpu"
            onClicked: root.tabKey = "system"
        }
        Tile {
            label: i18n("Router RAM")
            value: Math.round(root.system.mem_used_pct || 0) + ""
            unit: "%"
            valueColor: Style.heatStrong(root.system.mem_used_pct || 0, 80, 92, Kirigami.Theme)
            caption: root.kib((root.system.mem_total || 0) - (root.system.mem_avail || 0)) + " / " + root.kib(root.system.mem_total)
            icon: "memory"
            onClicked: root.tabKey = "system"
        }
    }

    PopCard {
        title: i18n("Speed test")
        icon: "speedometer"
        trailing: root.lastResult && root.lastResult.ts ? root.ago(root.lastResult.ts) : ""

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing * 2
            PopStat {
                label: i18n("↓ Down")
                value: root.testing && root.live ? root.speed(root.live.mbps).value : root.lastResult ? root.speed(root.lastResult.down_mbps).value : "—"
                unit: root.lastResult || root.live ? root.speed(root.testing && root.live ? root.live.mbps : root.lastResult.down_mbps).unit : ""
                color: root.downColor
            }
            PopStat {
                label: i18n("↑ Up")
                value: root.lastResult ? root.speed(root.lastResult.up_mbps).value : "—"
                unit: root.lastResult ? root.speed(root.lastResult.up_mbps).unit : ""
                color: root.upColor
            }
            PopStat {
                label: i18n("Ping")
                value: root.lastResult ? (root.lastResult.ping_ms || 0).toFixed(0) : "—"
                unit: root.lastResult ? "ms" : ""
            }
            Item { Layout.fillWidth: true }
            PlasmaComponents.Button {
                text: root.testing ? i18n("Testing…") : i18n("Run")
                icon.name: "media-playback-start"
                enabled: !root.testing
                onClicked: root.runSpeedTest()
            }
        }
    }

    PopCard {
        visible: root.radios.length > 0
        title: i18n("Radios")
        icon: "network-wireless"
        trailing: i18np("%1 client", "%1 clients", root.stations.length)

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: Kirigami.Units.largeSpacing

            Repeater {
                model: root.radios

                MouseArea {
                    id: radioPill
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    implicitHeight: radioColumn.implicitHeight + Kirigami.Units.smallSpacing * 3
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    opacity: modelData.on === false ? 0.5 : 1
                    onClicked: root.tabKey = "wifi"

                    Rectangle {
                        anchors.fill: parent
                        radius: Kirigami.Units.cornerRadius * 1.5
                        color: Qt.alpha(Kirigami.Theme.textColor, radioPill.containsMouse ? 0.08 : 0.04)
                    }
                    ColumnLayout {
                        id: radioColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: Kirigami.Units.smallSpacing * 2
                        spacing: Kirigami.Units.smallSpacing
                        RowLayout {
                            Layout.fillWidth: true
                            PlasmaComponents.Label {
                                text: radioPill.modelData.band
                                font.weight: Font.DemiBold
                                color: root.accent
                            }
                            Item { Layout.fillWidth: true }
                            PlasmaComponents.Label {
                                text: i18np("%1 client", "%1 clients", radioPill.modelData.clients || 0)
                                font: Kirigami.Theme.smallFont
                                opacity: 0.7
                            }
                            PlasmaComponents.Label {
                                text: Math.round(radioPill.modelData.temp || 0) + "°"
                                font: Kirigami.Theme.smallFont
                                color: Style.heat(radioPill.modelData.temp || 0, 75, 90, Kirigami.Theme)
                            }
                        }
                        PopBar {
                            Layout.fillWidth: true
                            label: i18n("Airtime")
                            value: radioPill.modelData.busy || 0
                            color: Style.heatStrong(radioPill.modelData.busy || 0, 40, 70, Kirigami.Theme) === Kirigami.Theme.positiveTextColor
                                   ? root.accent : Style.heatStrong(radioPill.modelData.busy || 0, 40, 70, Kirigami.Theme)
                            thickness: 4
                        }
                    }
                }
            }
        }
    }
}
