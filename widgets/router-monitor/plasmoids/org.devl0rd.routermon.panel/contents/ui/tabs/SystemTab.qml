import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import "../lib/PopStyle.js" as Style

PopScroll {
    id: systemTab

    readonly property var cpu: root.system.cpu || ({})
    readonly property var load: root.system.load || []

    PopCard {
        title: i18n("Router CPU")
        icon: "cpu"
        trailing: Math.round(systemTab.cpu.total || 0) + "%"
        trailingColor: Style.heatStrong(systemTab.cpu.total || 0, 60, 85, Kirigami.Theme)

        Sparkline {
            visible: Plasmoid.configuration.showCharts
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 3.2
            values: root.series("cpu")
            rangeMax: 100
            dangerFrom: 85
            tipText: v => Math.round(v) + "% CPU"
        }
        GridLayout {
            visible: Plasmoid.configuration.showPerCore
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: Kirigami.Units.smallSpacing
            Repeater {
                model: systemTab.cpu.cores || []
                PopBar {
                    required property var modelData
                    required property int index
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    label: i18n("Core %1", index + 1)
                    value: modelData
                    color: modelData >= 60 ? Style.heatStrong(modelData, 60, 85, Kirigami.Theme) : root.accent
                }
            }
        }
    }

    PopCard {
        title: i18n("Memory")
        icon: "memory"
        trailing: Math.round(root.system.mem_used_pct || 0) + "%"
        trailingColor: Style.heatStrong(root.system.mem_used_pct || 0, 80, 92, Kirigami.Theme)

        PopBar {
            Layout.fillWidth: true
            label: i18n("RAM")
            value: root.system.mem_used_pct || 0
            valueText: root.kib((root.system.mem_total || 0) - (root.system.mem_avail || 0)) + " / " + root.kib(root.system.mem_total)
            color: Style.hue("memory", Kirigami.Theme)
        }
        PopBar {
            visible: (root.system.swap_total || 0) > 0
            Layout.fillWidth: true
            label: i18n("Swap")
            value: root.system.swap_used_pct || 0
            valueText: root.kib(root.system.swap_used) + " / " + root.kib(root.system.swap_total)
            color: Style.hue("memory", Kirigami.Theme)
        }
    }

    PopCard {
        visible: Plasmoid.configuration.showTemps
        title: i18n("Temperatures")
        icon: "temperature-normal"

        Flow {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing * 1.5
            PopStat {
                label: i18n("CPU")
                value: Math.round(root.system.cpu_temp || 0) + ""
                unit: "°C"
                color: Style.heatStrong(root.system.cpu_temp || 0, 80, 95, Kirigami.Theme)
            }
            Repeater {
                model: root.radios
                PopStat {
                    required property var modelData
                    label: root.shortBand(modelData.band)
                    value: Math.round(modelData.temp || 0) + ""
                    unit: "°C"
                    color: Style.heatStrong(modelData.temp || 0, 75, 90, Kirigami.Theme)
                }
            }
        }
    }

    PopCard {
        title: i18n("Activity")
        icon: "view-statistics"

        RowLayout {
            Layout.fillWidth: true
            Repeater {
                model: [i18n("Load 1 min"), i18n("5 min"), i18n("15 min")]
                PopStat {
                    required property var modelData
                    required property int index
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    label: modelData
                    value: systemTab.load[index] !== undefined ? systemTab.load[index].toFixed(2) : "—"
                    scale: 1.1
                }
            }
            PopStat {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                label: i18n("Processes")
                value: (root.system.procs || "").split("/")[1] || "—"
                unit: (root.system.procs || "").indexOf("/") > 0 ? i18n("%1 running", root.system.procs.split("/")[0]) : ""
                scale: 1.1
            }
        }
        PopBar {
            Layout.fillWidth: true
            label: i18n("Tracked connections")
            value: root.system.conntrack_max ? (root.system.conntrack || 0) / root.system.conntrack_max * 100 : 0
            valueText: (root.system.conntrack || 0).toLocaleString(Qt.locale(), "f", 0) + " / " + (root.system.conntrack_max || 0).toLocaleString(Qt.locale(), "f", 0)
            color: root.accent
        }
        RowLayout {
            Layout.fillWidth: true
            visible: root.security.wrs_protect !== undefined
            PlasmaComponents.Label {
                text: i18n("Router security")
                opacity: 0.65
                Layout.fillWidth: true
            }
            Pill {
                text: root.security.wrs_protect ? i18n("Intrusion protection on") : i18n("Intrusion protection off")
                tint: root.security.wrs_protect ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
                strength: root.security.wrs_protect ? 0.18 : 0.08
            }
            Pill {
                text: root.security.wrs_mals ? i18n("Malware blocking on") : i18n("Malware blocking off")
                tint: root.security.wrs_mals ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
                strength: root.security.wrs_mals ? 0.18 : 0.08
            }
        }
    }

    PopCard {
        title: i18n("Actions")
        icon: "system-run"

        Flow {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing
            PopConfirm {
                visible: Plasmoid.configuration.showReboot
                label: i18n("Reboot router")
                iconName: "system-reboot"
                onConfirmed: root.ctlRun("reboot")
            }
            PopConfirm {
                visible: Plasmoid.configuration.showRestartWifi
                label: i18n("Restart WiFi")
                iconName: "network-wireless"
                onConfirmed: root.ctlRun("restart-wifi")
            }
            PlasmaComponents.Button {
                visible: Plasmoid.configuration.showWebUI && !!root.info.admin_url
                text: i18n("Open web UI")
                icon.name: "internet-web-browser"
                onClicked: root.launch("xdg-open " + root.info.admin_url)
            }
            PlasmaComponents.Button {
                text: root.paused ? i18n("Resume monitoring") : i18n("Pause monitoring")
                icon.name: root.paused ? "media-playback-start" : "media-playback-pause"
                onClicked: root.ctlRun("pause toggle")
            }
        }
    }
}
