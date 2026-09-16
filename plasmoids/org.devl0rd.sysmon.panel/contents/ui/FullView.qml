import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import "lib"
import "lib/PopStyle.js" as Style
import "lib/Format.js" as Fmt
import "lib/Highlight.js" as Highlight

Item {
    id: full

    Layout.minimumWidth: Kirigami.Units.gridUnit * 13
    Layout.minimumHeight: Kirigami.Units.gridUnit * 12
    Layout.preferredWidth: Kirigami.Units.gridUnit * 26
    Layout.preferredHeight: Kirigami.Units.gridUnit * 42

    Loader {
        id: loader
        anchors.fill: parent
        active: root.popupAlive
        sourceComponent: dashboard
        onLoaded: if (root.expanded) item.focusSearch()
    }

    Connections {
        target: root
        function onExpandedChanged() {
            if (root.expanded && loader.item)
                loader.item.focusSearch()
        }
    }

    component CoreBars: RowLayout {
        id: bars
        property var values: []
        property string prefix
        property int highlight: -1
        property bool groupMatched: false
        Layout.fillWidth: true
        Layout.preferredHeight: Kirigami.Units.gridUnit * 2.2
        spacing: 3
        Repeater {
            model: bars.values.length
            Item {
                id: coreBar
                required property int index
                readonly property real value: bars.values[index] || 0
                readonly property bool marked: bars.highlight === index + 1 || (bars.groupMatched && bars.highlight < 0)
                Layout.fillWidth: true
                Layout.fillHeight: true
                Rectangle {
                    anchors.fill: parent
                    radius: 2
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
                    border.width: coreBar.marked ? 1.5 : 0
                    border.color: Kirigami.Theme.highlightColor
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: Math.max(2, parent.height * Math.min(1, coreBar.value / 100))
                    radius: 2
                    color: Fmt.grad(coreBar.value)
                    Behavior on height { NumberAnimation { duration: 350; easing.type: Easing.OutCubic } }
                    Behavior on color { ColorAnimation { duration: 350 } }
                }
                HoverHandler { id: coreHover }
                QQC2.ToolTip.visible: coreHover.hovered
                QQC2.ToolTip.text: i18n("%1 %2 · %3%", bars.prefix, index + 1, Math.round(coreBar.value))
                QQC2.ToolTip.delay: 200
            }
        }
    }

    component Pill: Rectangle {
        property alias text: pillLabel.text
        property string tip
        implicitWidth: pillLabel.implicitWidth + Kirigami.Units.smallSpacing * 3
        implicitHeight: pillLabel.implicitHeight + Kirigami.Units.smallSpacing
        radius: height / 2
        color: Qt.alpha(Kirigami.Theme.textColor, 0.07)
        PlasmaComponents.Label {
            id: pillLabel
            anchors.centerIn: parent
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            font.features: { "tnum": 1 }
            opacity: 0.8
        }
        HoverHandler { id: pillHover }
        QQC2.ToolTip.visible: pillHover.hovered && tip !== ""
        QQC2.ToolTip.text: tip
        QQC2.ToolTip.delay: 300
    }

    Component {
        id: dashboard

        PopupShell {
            id: shell

            readonly property string query: searchText.trim().toLowerCase()
            readonly property var coreQuery: {
                const match = query.match(/^(p-?|e-?)?cores?\s*(\d+)?$/)
                if (!match)
                    return null
                const group = match[1] ? match[1].charAt(0) : ""
                return { group: group, index: match[2] ? parseInt(match[2]) : -1 }
            }
            readonly property bool hybrid: root.cpu.hybrid === true

            readonly property var collapsed: Plasmoid.configuration.collapsedCards.split(",").filter(name => name !== "")
            function isCollapsed(name) {
                return query === "" && collapsed.indexOf(name) >= 0
            }
            function toggleCollapsed(name) {
                const next = collapsed.filter(entry => entry !== name)
                if (next.length === collapsed.length)
                    next.push(name)
                Plasmoid.configuration.collapsedCards = next.join(",")
            }

            function matchesWords(words) {
                if (query === "")
                    return true
                return words.some(word => word.indexOf(query) === 0 || query.indexOf(word) === 0)
            }
            function metricMatch(metrics) {
                if (query === "")
                    return -1
                for (let i = 0; i < metrics.length; ++i) {
                    if (metrics[i].keywords.some(word => word.indexOf(query) === 0))
                        return i
                }
                return -1
            }
            readonly property int cpuMetricMatch: metricMatch(root.cpuMetrics)
            readonly property int gpuMetricMatch: metricMatch(root.gpuMetrics)
            readonly property bool cpuMatch: cpuMetricMatch >= 0 || matchesWords(["cpu", "processor", root.cpuShort().toLowerCase()])
            readonly property bool coresMatch: Plasmoid.configuration.showPerCore && (query === "" || coreQuery !== null || matchesWords(["cores", "threads", "p-core", "e-core"]))
            readonly property bool gpuMatch: root.gpu !== null && Plasmoid.configuration.showGpu
                                             && (gpuMetricMatch >= 0 || matchesWords(["gpu", "graphics", "vram", "video", "memory clock", root.gpuShort(root.gpu ? root.gpu.name : "").toLowerCase()]))
            readonly property bool memMatch: matchesWords(["memory", "ram", "swap"])
            readonly property var matchedCards: [cpuMatch ? cpuCard : null, coresMatch ? coresCard : null, gpuMatch ? gpuCard : null, memMatch ? memCard : null].filter(card => card !== null)

            anchors.fill: parent
            icon: root.panelIcon
            title: root.snap.host || i18n("System Monitor")
            subtitle: {
                const parts = []
                if (root.cpuShort())
                    parts.push(root.cpuShort())
                if (root.snap.uptime)
                    parts.push(i18n("up %1", Fmt.duration(root.snap.uptime)))
                return parts.join("  ·  ")
            }
            statusColor: root.collectorAlive ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.disabledTextColor
            statusText: root.collectorAlive ? i18n("Live") : i18n("Collector stopped")
            searchPlaceholder: i18n("Search CPU, cores, GPU, memory…")
            matchCount: query === "" ? -1 : matchedCards.length
            onSearchAccepted: if (matchedCards.length > 0) scroll.scrollTo(matchedCards[0])
            onCloseRequested: root.expanded = false

            headerExtra: [
                Pill {
                    visible: !!root.snap.load
                    text: root.snap.load ? root.snap.load.map(v => v.toFixed(1)).join("  ") : ""
                    tip: i18n("Load average over 1, 5 and 15 minutes")
                }
            ]
            headerActions: [
                PlasmaComponents.ToolButton {
                    icon.name: "utilities-system-monitor"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Open System Monitor")
                    onClicked: root.run("plasma-systemmonitor")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                },
                PlasmaComponents.ToolButton {
                    icon.name: "configure"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Configure…")
                    onClicked: Plasmoid.internalAction("configure").trigger()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                }
            ]

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: !root.snap.cpu
                iconName: root.panelIcon
                text: i18n("Waiting for data")
                explanation: i18n("The system monitor collector hasn't written a snapshot yet. Run install.sh if it isn't installed.")
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: !!root.snap.cpu && shell.query !== "" && shell.matchedCards.length === 0
                iconName: "edit-find"
                text: i18n("No matches")
                explanation: i18n("Try cpu, temp, core 3, gpu, vram or swap")
            }

            PopScroll {
                id: scroll
                anchors.fill: parent
                visible: !!root.snap.cpu

                RowLayout {
                    visible: shell.query === ""
                    Layout.fillWidth: true
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    Layout.bottomMargin: Kirigami.Units.smallSpacing
                    spacing: 0

                    Item { Layout.fillWidth: true }
                    PopRing {
                        value: root.cpu.total || 0
                        label: i18n("CPU")
                        caption: Fmt.temp(root.cpu.temp) + " · " + (root.cpu.freq || 0).toFixed(1) + " GHz"
                        color: Style.heatStrong(root.cpu.total || 0, 60, 85, Kirigami.Theme)
                        onClicked: { scroll.scrollTo(cpuCard); cpuCard.flash() }
                    }
                    Item { Layout.fillWidth: true }
                    PopRing {
                        visible: root.gpu !== null && Plasmoid.configuration.showGpu
                        value: root.gpu ? root.gpu.util || 0 : 0
                        label: i18n("GPU")
                        caption: root.gpu ? Fmt.temp(root.gpu.temp) + " · " + Math.round(root.gpu.power || 0) + " W" : ""
                        color: Style.heatStrong(root.gpu ? root.gpu.util || 0 : 0, 60, 85, Kirigami.Theme)
                        onClicked: { scroll.scrollTo(gpuCard); gpuCard.flash() }
                    }
                    Item { Layout.fillWidth: true; visible: root.gpu !== null && Plasmoid.configuration.showGpu }
                    PopRing {
                        value: root.mem.pct || 0
                        label: i18n("RAM")
                        caption: Style.bytes(root.mem.used) + " / " + Style.bytes(root.mem.total)
                        color: Style.heatStrong(root.mem.pct || 0, 75, 90, Kirigami.Theme)
                        onClicked: { scroll.scrollTo(memCard); memCard.flash() }
                    }
                    Item { Layout.fillWidth: true }
                }

                PopCard {
                    id: cpuCard
                    collapsible: shell.query === ""
                    collapsed: shell.isCollapsed("cpu")
                    onCollapseToggled: shell.toggleCollapsed("cpu")
                    visible: shell.cpuMatch
                    title: i18n("CPU")
                    icon: "cpu"
                    trailing: cpuTabs.current.fmt ? cpuTabs.current.fmt(cpuTabs.current.get(root.cpu)) : ""
                    trailingColor: cpuTabs.current.color ? cpuTabs.current.color(cpuTabs.current.get(root.cpu)) : Kirigami.Theme.textColor

                    PopMetricTabs {
                        id: cpuTabs
                        metrics: root.cpuMetrics
                        source: root.cpu
                        tick: root.tick
                        valuesFor: key => root.series("cpu." + key)
                        chartVisible: Plasmoid.configuration.showCharts
                        selected: shell.cpuMetricMatch >= 0 ? shell.cpuMetricMatch : Plasmoid.configuration.cpuTab
                        extraStats: root.snap.ncpu ? i18np("%1 thread", "%1 threads", root.snap.ncpu) : ""
                        onUserSelected: index => Plasmoid.configuration.cpuTab = index
                    }
                }

                PopCard {
                    id: coresCard
                    collapsible: shell.query === ""
                    collapsed: shell.isCollapsed("cores")
                    onCollapseToggled: shell.toggleCollapsed("cores")
                    visible: shell.coresMatch
                    title: i18n("Cores")
                    icon: "view-grid"
                    trailing: shell.hybrid ? i18n("%1 P · %2 E", (root.cpu.p || []).length, (root.cpu.e || []).length) : i18np("%1 thread", "%1 threads", (root.cpu.cores || []).length)

                    PopBar {
                        visible: shell.hybrid
                        Layout.fillWidth: true
                        label: i18n("Performance cores")
                        value: root.cpu.p_total || 0
                        valueText: Math.round(root.cpu.p_total || 0) + "%  ·  " + (root.cpu.p_freq || 0).toFixed(2) + " GHz"
                        color: root.accent
                    }
                    CoreBars {
                        visible: shell.hybrid
                        values: root.cpu.p || []
                        prefix: i18n("P-core")
                        groupMatched: shell.coreQuery !== null && shell.coreQuery.group === "p"
                        highlight: shell.coreQuery && shell.coreQuery.group !== "e" && shell.coreQuery.index > 0 ? shell.coreQuery.index : -1
                    }
                    PopBar {
                        visible: shell.hybrid
                        Layout.fillWidth: true
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        label: i18n("Efficiency cores")
                        value: root.cpu.e_total || 0
                        valueText: Math.round(root.cpu.e_total || 0) + "%  ·  " + (root.cpu.e_freq || 0).toFixed(2) + " GHz"
                        color: Style.hue("fan", Kirigami.Theme)
                    }
                    CoreBars {
                        visible: shell.hybrid
                        values: root.cpu.e || []
                        prefix: i18n("E-core")
                        groupMatched: shell.coreQuery !== null && shell.coreQuery.group === "e"
                        highlight: shell.coreQuery && shell.coreQuery.group === "e" && shell.coreQuery.index > 0 ? shell.coreQuery.index : -1
                    }
                    CoreBars {
                        visible: !shell.hybrid
                        values: root.cpu.cores || []
                        prefix: i18n("Core")
                        highlight: shell.coreQuery && shell.coreQuery.index > 0 ? shell.coreQuery.index : -1
                    }
                }

                PopCard {
                    id: gpuCard
                    collapsible: shell.query === ""
                    collapsed: shell.isCollapsed("gpu")
                    onCollapseToggled: shell.toggleCollapsed("gpu")
                    visible: shell.gpuMatch
                    title: root.gpu ? root.gpuShort(root.gpu.name) : i18n("GPU")
                    icon: "video-display"
                    trailing: root.gpu && gpuTabs.current.fmt ? gpuTabs.current.fmt(gpuTabs.current.get(root.gpu)) : ""
                    trailingColor: root.gpu && gpuTabs.current.color ? gpuTabs.current.color(gpuTabs.current.get(root.gpu)) : Kirigami.Theme.textColor

                    PopMetricTabs {
                        id: gpuTabs
                        metrics: root.gpuMetrics
                        source: root.gpu || ({})
                        tick: root.tick
                        valuesFor: key => root.series("gpu." + key)
                        chartVisible: Plasmoid.configuration.showCharts
                        selected: shell.gpuMetricMatch >= 0 ? shell.gpuMetricMatch : Plasmoid.configuration.gpuTab
                        onUserSelected: index => Plasmoid.configuration.gpuTab = index
                    }
                    PopBar {
                        Layout.fillWidth: true
                        label: i18n("VRAM")
                        value: root.gpu ? root.gpu.vram_pct || 0 : 0
                        valueText: root.gpu ? Style.bytes(root.gpu.vram_used) + " / " + Style.bytes(root.gpu.vram_total) : ""
                        color: (root.gpu ? root.gpu.vram_pct || 0 : 0) >= 75 ? Style.heatStrong(root.gpu.vram_pct, 75, 90, Kirigami.Theme) : Style.hue("memory", Kirigami.Theme)
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Repeater {
                            model: root.gpu ? [
                                { label: i18n("Memory clock"), value: Math.round(root.gpu.clock_mem || 0), unit: "MHz" },
                                { label: i18n("Power limit"), value: Math.round(root.gpu.power_max || 0), unit: "W" },
                                { label: i18n("Max clock"), value: Math.round(root.gpu.clock_max || 0), unit: "MHz" }
                            ] : []
                            PopStat {
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                label: modelData.label
                                value: modelData.value > 0 ? modelData.value + "" : "—"
                                unit: modelData.value > 0 ? modelData.unit : ""
                                scale: 1.0
                            }
                        }
                    }
                }

                PopCard {
                    id: memCard
                    collapsible: shell.query === ""
                    collapsed: shell.isCollapsed("memory")
                    onCollapseToggled: shell.toggleCollapsed("memory")
                    visible: shell.memMatch
                    title: i18n("Memory")
                    icon: "memory"
                    trailing: Math.round(root.mem.pct || 0) + "%"
                    trailingColor: Style.heatStrong(root.mem.pct || 0, 75, 90, Kirigami.Theme)

                    Sparkline {
                        visible: Plasmoid.configuration.showCharts
                        Layout.fillWidth: true
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 3
                        values: root.series("mem.usage")
                        rangeMax: 100
                        dangerFrom: 90
                        tipText: v => Math.round(v) + "% RAM"
                    }
                    PopBar {
                        Layout.fillWidth: true
                        label: i18n("RAM")
                        value: root.mem.pct || 0
                        valueText: Style.bytes(root.mem.used) + " / " + Style.bytes(root.mem.total)
                        color: Style.hue("memory", Kirigami.Theme)
                    }
                    PopBar {
                        visible: (root.mem.swap_total || 0) > 0
                        Layout.fillWidth: true
                        label: i18n("Swap")
                        value: root.mem.swap_pct || 0
                        valueText: Style.bytes(root.mem.swap_used) + " / " + Style.bytes(root.mem.swap_total)
                        color: Style.hue("memory", Kirigami.Theme)
                    }
                }

                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    Layout.topMargin: -Kirigami.Units.smallSpacing
                    horizontalAlignment: Text.AlignHCenter
                    font: Kirigami.Theme.smallFont
                    opacity: 0.5
                    color: root.collectorAlive ? Kirigami.Theme.textColor : Kirigami.Theme.negativeTextColor
                    text: root.collectorAlive
                          ? i18n("Updated %1", Qt.formatTime(new Date((root.snap.ts || 0) * 1000), "hh:mm:ss"))
                          : i18n("Collector not running")
                }
            }
        }
    }
}
