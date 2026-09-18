import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kquickcontrols as KQuickControls
import org.kde.plasma.plasmoid

Kirigami.FormLayout {
    property alias cfg_accentColor: accent.text
    property alias cfg_showCharts: showCharts.checked
    property alias cfg_showPerCore: showPerCore.checked
    property alias cfg_showTemps: showTemps.checked
    property alias cfg_showReboot: showReboot.checked
    property alias cfg_showRestartWifi: showRestartWifi.checked
    property alias cfg_showWebUI: showWebUI.checked
    property alias cfg_maxMbps: maxMbps.value
    property alias cfg_showInterference: showInterference.checked
    property alias cfg_showProtection: showProtection.checked
    property alias cfg_showOpenUI: showOpenUI.checked
    property alias cfg_showChart: showChart.checked
    property alias cfg_showTopLists: showTopLists.checked
    property string cfg_sortBy
    property alias cfg_sshUser: sshUser.text
    property string cfg_pinnedMacs
    property alias cfg_planDownMbps: planDownMbps.value
    property string cfg_lastResult
    property real cfg_peakDown
    property real cfg_peakUp
    property string cfg_compactExtra
    property alias cfg_panelShrink: panelShrink.checked
    property string cfg_panelDetail
    property alias cfg_middleClickPause: middleClickPause.checked
    property string cfg_defaultTab
    property string cfg_currentTab
    property string cfg_clientFilter
    property string cfg_speedHistory
    property alias cfg_rememberTab: rememberTab.checked
    readonly property bool tabbed: Plasmoid.metaData.pluginId === "org.devl0rd.routermon.panel"

    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Panel also shows:")
        textRole: "text"
        valueRole: "value"
        model: [
            { text: i18n("Ping"), value: "ping" },
            { text: i18n("Devices online"), value: "clients" },
            { text: i18n("DNS blocked %"), value: "blocked" },
            { text: i18n("Nothing else"), value: "none" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(cfg_compactExtra))
        onActivated: cfg_compactExtra = currentValue
    }
    QQC2.CheckBox { id: panelShrink; Kirigami.FormData.label: i18n("Panel space:"); text: i18n("Shrink to fit the panel") }
    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Detail level:")
        textRole: "text"
        valueRole: "value"
        model: [
            { text: i18n("Automatic"), value: "auto" },
            { text: i18n("Label and value"), value: "full" },
            { text: i18n("Icon and value"), value: "medium" },
            { text: i18n("Value only"), value: "small" },
            { text: i18n("Indicator only"), value: "tiny" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(cfg_panelDetail))
        onActivated: cfg_panelDetail = currentValue
    }
    QQC2.CheckBox { id: middleClickPause; text: i18n("Middle-click pauses and resumes monitoring") }

    Item { Kirigami.FormData.isSection: true }

    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Open on:")
        visible: tabbed
        textRole: "text"
        valueRole: "value"
        enabled: !rememberTab.checked
        model: [
            { text: i18n("Overview"), value: "overview" },
            { text: i18n("Network"), value: "network" },
            { text: i18n("WiFi"), value: "wifi" },
            { text: i18n("Clients"), value: "clients" },
            { text: i18n("DNS"), value: "dns" },
            { text: i18n("Speed"), value: "speed" },
            { text: i18n("System"), value: "system" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(cfg_defaultTab))
        onActivated: cfg_defaultTab = currentValue
    }
    QQC2.CheckBox { id: rememberTab; visible: tabbed; text: i18n("Reopen on the last tab") }

    QQC2.CheckBox { id: showCharts; Kirigami.FormData.label: i18n("Show:"); text: i18n("History graphs") }
    QQC2.CheckBox { id: showPerCore; text: i18n("Router per-core bars") }
    QQC2.CheckBox { id: showTemps; text: i18n("Temperatures") }
    QQC2.CheckBox { id: showInterference; text: i18n("WiFi airtime") }
    QQC2.CheckBox { id: showProtection; text: i18n("AdGuard protection switch") }
    QQC2.CheckBox { id: showOpenUI; text: i18n("Open AdGuard button") }
    QQC2.CheckBox { id: showChart; text: i18n("DNS query graph") }
    QQC2.CheckBox { id: showTopLists; text: i18n("DNS top lists") }
    QQC2.CheckBox { id: showWebUI; text: i18n("Open router web UI button") }
    QQC2.CheckBox { id: showReboot; text: i18n("Reboot button") }
    QQC2.CheckBox { id: showRestartWifi; text: i18n("Restart WiFi button") }

    Item { Kirigami.FormData.isSection: true }

    RowLayout {
        Kirigami.FormData.label: i18n("Speed graph scale:")
        QQC2.SpinBox { id: maxMbps; from: 0; to: 10000; stepSize: 50 }
        QQC2.Label { text: maxMbps.value === 0 ? i18n("automatic") : i18n("Mb/s"); opacity: 0.6 }
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Internet plan:")
        QQC2.SpinBox { id: planDownMbps; from: 0; to: 10000; stepSize: 50 }
        QQC2.Label { text: planDownMbps.value === 0 ? i18n("gauge tops out at 1000") : i18n("Mb/s download"); opacity: 0.6 }
    }
    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Sort clients:")
        textRole: "text"
        valueRole: "value"
        model: [
            { text: i18n("By traffic"), value: "traffic" },
            { text: i18n("By signal"), value: "signal" },
            { text: i18n("By name"), value: "name" },
            { text: i18n("By IP"), value: "ip" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(cfg_sortBy))
        onActivated: cfg_sortBy = currentValue
    }
    QQC2.TextField {
        id: sshUser
        Kirigami.FormData.label: i18n("SSH user:")
        placeholderText: i18n("your user name")
    }

    Item { Kirigami.FormData.isSection: true }

    RowLayout {
        Kirigami.FormData.label: i18n("Accent colour:")
        QQC2.CheckBox {
            id: useAccent
            text: i18n("Custom")
            checked: accent.text !== ""
            onToggled: if (!checked) accent.text = ""
        }
        KQuickControls.ColorButton {
            enabled: useAccent.checked
            color: accent.text !== "" ? accent.text : Kirigami.Theme.highlightColor
            onColorChanged: if (useAccent.checked) accent.text = color
        }
        QQC2.Label { id: accent; visible: false; text: "" }
    }
}
