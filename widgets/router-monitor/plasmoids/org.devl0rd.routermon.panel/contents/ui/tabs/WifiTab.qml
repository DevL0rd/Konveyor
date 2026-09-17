import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import "../lib/PopStyle.js" as Style
import "../lib/Format.js" as Fmt

PopScroll {
    id: wifiTab

    property var openStations: ({})

    Repeater {
        model: root.radios

        PopCard {
            id: radioCard
            required property var modelData
            readonly property var radio: modelData
            readonly property var bandStations: root.stations.filter(station => station.band === radio.band)
            readonly property bool is5g: (radio.band || "").indexOf("5GHz") === 0

            opacity: radio.on === false ? 0.55 : 1
            title: radio.band
            icon: "network-wireless"
            trailing: i18np("%1 client", "%1 clients", radio.clients || 0)

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                PlasmaComponents.Label {
                    text: radioCard.radio.ssid || i18n("Hidden network")
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Pill {
                    visible: radioCard.radio.on === false
                    text: i18n("Off")
                    strength: 0.14
                }
                Pill {
                    visible: radioCard.is5g
                    text: radioCard.radio.dfs === "IDLE" ? i18n("No DFS") : i18n("DFS: %1", radioCard.radio.dfs)
                    tint: radioCard.radio.dfs === "IDLE" ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.neutralTextColor
                    strength: 0.18
                }
            }

            RowLayout {
                Layout.fillWidth: true
                PopStat {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    label: i18n("Channel")
                    value: (radioCard.radio.chan || 0) + ""
                    unit: radioCard.radio.width || ""
                    scale: 1.1
                }
                PopStat {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    label: i18n("Noise")
                    value: (radioCard.radio.noise || 0) + ""
                    unit: "dBm"
                    scale: 1.1
                }
                PopStat {
                    visible: Plasmoid.configuration.showTemps
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    label: i18n("Temp")
                    value: Math.round(radioCard.radio.temp || 0) + ""
                    unit: "°C"
                    color: Style.heatStrong(radioCard.radio.temp || 0, 75, 90, Kirigami.Theme)
                    scale: 1.1
                }
                PopStat {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    label: i18n("Power")
                    value: radioCard.radio.txpower !== undefined ? radioCard.radio.txpower + "" : "—"
                    unit: radioCard.radio.txpower !== undefined ? "%" : ""
                    scale: 1.1
                }
            }

            PopBar {
                visible: Plasmoid.configuration.showInterference
                Layout.fillWidth: true
                label: i18n("Airtime in use")
                value: radioCard.radio.busy || 0
                valueText: Math.round(radioCard.radio.busy || 0) + "%" + ((radioCard.radio.glitch || 0) > 100 ? i18n(" · %1 glitches", radioCard.radio.glitch) : "")
                color: (radioCard.radio.busy || 0) >= 40 ? Style.heatStrong(radioCard.radio.busy, 40, 70, Kirigami.Theme) : root.accent
            }

            MouseArea {
                visible: radioCard.bandStations.length > 0
                Layout.fillWidth: true
                implicitHeight: stationsHeader.implicitHeight + Kirigami.Units.smallSpacing
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    const next = Object.assign({}, wifiTab.openStations)
                    next[radioCard.radio.band] = !next[radioCard.radio.band]
                    wifiTab.openStations = next
                }
                RowLayout {
                    id: stationsHeader
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    Kirigami.Icon {
                        source: wifiTab.openStations[radioCard.radio.band] ? "arrow-down" : "arrow-right"
                        Layout.preferredWidth: Kirigami.Units.iconSizes.small
                        Layout.preferredHeight: Kirigami.Units.iconSizes.small
                        opacity: 0.6
                    }
                    PlasmaComponents.Label {
                        text: i18np("%1 connected device", "%1 connected devices", radioCard.bandStations.length)
                        opacity: 0.75
                        Layout.fillWidth: true
                    }
                }
            }

            Repeater {
                model: wifiTab.openStations[radioCard.radio.band] ? radioCard.bandStations : []
                RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.smallSpacing
                    SignalBars {
                        level: Fmt.rssiBars(modelData.rssi)
                        activeColor: Fmt.rssiColor(modelData.rssi, Kirigami.Theme)
                        tip: modelData.rssi + " dBm"
                    }
                    PlasmaComponents.Label {
                        text: root.nameForMac(modelData.mac)
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    PlasmaComponents.Label {
                        text: modelData.rssi + " dBm"
                        font: Kirigami.Theme.smallFont
                        opacity: 0.6
                    }
                    PlasmaComponents.Label {
                        text: i18n("link ↓%1 ↑%2 Mb/s", Math.round(modelData.tx_mbps || 0), Math.round(modelData.rx_mbps || 0))
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        font.features: { "tnum": 1 }
                        opacity: 0.6
                    }
                    PlasmaComponents.Label {
                        text: Fmt.rate(modelData.traffic_bps)
                        font: Kirigami.Theme.smallFont
                        color: modelData.traffic_bps > 0 ? root.accent : Kirigami.Theme.textColor
                        opacity: modelData.traffic_bps > 0 ? 1 : 0.5
                    }
                }
            }
        }
    }

    PopConfirm {
        visible: Plasmoid.configuration.showRestartWifi
        Layout.alignment: Qt.AlignHCenter
        label: i18n("Restart WiFi")
        iconName: "network-wireless"
        onConfirmed: root.ctlRun("restart-wifi")
    }
}
