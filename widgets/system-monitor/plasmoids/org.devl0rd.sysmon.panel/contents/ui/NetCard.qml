import QtQuick
import org.kde.plasma.plasmoid
import "lib"
import "lib/Format.js" as Fmt

PopCard {
    collapsible: shell.query === ""
    collapsed: shell.isCollapsed("network")
    onCollapseToggled: shell.toggleCollapsed("network")
    visible: shell.netMatch
    title: i18n("Network")
    icon: "network-wired"
    trailing: root.net ? "↓ " + Fmt.rate(root.net.down) + "  ↑ " + Fmt.rate(root.net.up) : ""

    PopMetricTabs {
        metrics: root.netMetrics
        source: root.net || ({})
        tick: root.tick
        valuesFor: key => root.series("net." + key)
        chartVisible: Plasmoid.configuration.showCharts
        selected: shell.netMetricMatch >= 0 ? shell.netMetricMatch : Plasmoid.configuration.netTab
        extraStats: root.net ? (root.net.ifaces || []).join(", ") : ""
        onUserSelected: index => Plasmoid.configuration.netTab = index
    }
}
