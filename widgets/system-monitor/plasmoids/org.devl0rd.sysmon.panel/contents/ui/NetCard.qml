import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import "lib"

PopCard {
    collapsible: shell.query === ""
    collapsed: shell.isCollapsed("network")
    onCollapseToggled: shell.toggleCollapsed("network")
    visible: shell.netMatch
    title: i18n("Network")
    icon: root.netWireless ? "network-wireless" : "network-wired"
    trailing: root.net && netTabs.current.fmt ? netTabs.current.fmt(netTabs.current.get(root.net)) : ""
    trailingColor: root.net && netTabs.current.color ? netTabs.current.color(netTabs.current.get(root.net), Kirigami.Theme) : Kirigami.Theme.textColor

    PopMetricTabs {
        id: netTabs
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
