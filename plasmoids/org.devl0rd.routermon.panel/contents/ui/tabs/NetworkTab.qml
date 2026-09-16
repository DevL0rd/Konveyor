import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import "../lib/PopStyle.js" as Style

PopScroll {
    id: networkTab

    component InfoRow: RowLayout {
        id: infoRow
        property string label
        property string value
        property bool copyable: true
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        PlasmaComponents.Label {
            text: infoRow.label
            opacity: 0.6
            Layout.preferredWidth: Kirigami.Units.gridUnit * 6
        }
        PlasmaComponents.Label {
            text: infoRow.value || "—"
            font.features: { "tnum": 1 }
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        PlasmaComponents.ToolButton {
            visible: infoRow.copyable && infoRow.value !== ""
            icon.name: "edit-copy"
            display: PlasmaComponents.AbstractButton.IconOnly
            implicitWidth: Kirigami.Units.iconSizes.small + Kirigami.Units.smallSpacing * 2
            implicitHeight: implicitWidth
            text: i18n("Copy")
            onClicked: root.copy(infoRow.value)
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.text: text
        }
    }

    component SpeedChart: PopCard {
        id: speedCard
        property string key
        property color lineColor
        property real current
        trailing: root.speedText(current)
        trailingColor: lineColor
        Sparkline {
            visible: Plasmoid.configuration.showCharts
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 3.2
            values: root.series(speedCard.key)
            lineColor: speedCard.lineColor
            gradient: false
            rangeMax: Plasmoid.configuration.maxMbps
            rangeFloor: 1
            tipText: v => root.speedText(v)
        }
    }

    PopCard {
        title: i18n("WAN")
        icon: "network-wired-activated"
        trailing: (root.network.wan_proto || "").toUpperCase()

        RowLayout {
            Layout.fillWidth: true
            Pill {
                text: root.wanUp ? i18n("Connected") : i18n("Down")
                tint: root.wanUp ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                strength: 0.18
            }
            Item { Layout.fillWidth: true }
        }
        InfoRow { label: i18n("Public IP"); value: root.network.wan_ip || "" }
        InfoRow { label: i18n("Gateway"); value: root.network.wan_gw || "" }
        InfoRow { label: i18n("DNS servers"); value: (root.network.wan_dns || "").split(" ").filter(s => s).join(", ") }
        InfoRow { label: i18n("Router"); value: root.info.lan_ip || "" }
    }

    SpeedChart {
        title: i18n("Download")
        icon: "go-down"
        key: "down"
        lineColor: root.downColor
        current: root.network.down_mbps || 0
    }

    SpeedChart {
        title: i18n("Upload")
        icon: "go-up"
        key: "up"
        lineColor: root.upColor
        current: root.network.up_mbps || 0
    }

    PopCard {
        title: i18n("Connection quality")
        icon: "network-connect"

        RowLayout {
            Layout.fillWidth: true
            PopStat {
                Layout.fillWidth: true
                label: i18n("Latency (WAN)")
                value: (root.network.ping_rtt || 0).toFixed(1)
                unit: "ms"
                color: Style.heatStrong(root.network.ping_rtt || 0, 40, 100, Kirigami.Theme)
            }
            PopStat {
                Layout.fillWidth: true
                label: i18n("Packet loss")
                value: (root.network.ping_loss || 0) + ""
                unit: "%"
                color: Style.heatStrong(root.network.ping_loss || 0, 1, 5, Kirigami.Theme)
            }
        }
        Sparkline {
            visible: Plasmoid.configuration.showCharts
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 2.4
            values: root.series("ping")
            rangeFloor: 20
            dangerFrom: 100
            tipText: v => v.toFixed(1) + " ms"
        }
    }

    PopCard {
        visible: (root.network.ports || []).length > 0
        title: i18n("Wired ports")
        icon: "network-wired"
        trailing: i18np("%1 linked", "%1 linked", (root.network.ports || []).filter(port => /^Up/.test(port.link || "")).length)

        Repeater {
            model: root.network.ports || []
            RowLayout {
                required property var modelData
                readonly property bool up: /^Up/.test(modelData.link || "")
                readonly property string speed: {
                    const match = (modelData.link || "").match(/Speed:\s*([\w.]+)/)
                    return match ? match[1] : ""
                }
                readonly property string duplex: {
                    const match = (modelData.link || "").match(/Duplex:\s*(\w+)/)
                    return match ? (match[1] === "FD" ? i18n("full duplex") : i18n("half duplex")) : ""
                }
                Layout.fillWidth: true
                Rectangle {
                    width: Kirigami.Units.smallSpacing * 1.6
                    height: width
                    radius: width / 2
                    color: parent.up ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.disabledTextColor
                }
                PlasmaComponents.Label {
                    text: modelData.port
                    font.weight: Font.DemiBold
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 3
                }
                PlasmaComponents.Label {
                    text: parent.up ? [parent.speed, parent.duplex].filter(s => s).join(" · ") : i18n("not connected")
                    opacity: parent.up ? 0.85 : 0.5
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }
        }
    }

    PopCard {
        visible: !!(root.network.ifaces || {}).br0
        title: i18n("Local network")
        icon: "network-workgroup"
        trailing: "↓ " + root.speedText(((root.network.ifaces || {}).br0 || {}).tx_mbps) + "  ↑ " + root.speedText(((root.network.ifaces || {}).br0 || {}).rx_mbps)

        Sparkline {
            visible: Plasmoid.configuration.showCharts
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 2.6
            values: root.series("lan.tx")
            values2: root.series("lan.rx")
            lineColor: root.downColor
            lineColor2: root.upColor
            gradient: false
            rangeFloor: 1
            tipText: v => root.speedText(v)
        }
    }
}
