import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import "../lib"
import "../lib/PopStyle.js" as Style

Item {
    id: dnsTab

    readonly property var dns: root.dns

    component TopList: PopCard {
        id: topCard
        property var entries: []
        property color barColor: root.accent
        readonly property real most: entries.length > 0 ? Math.max.apply(null, entries.map(entry => entry.count || 0)) : 1
        trailing: entries.length > 0 ? i18np("%1 entry", "%1 entries", entries.length) : ""

        Repeater {
            model: topCard.entries
            Item {
                required property var modelData
                Layout.fillWidth: true
                implicitHeight: entryRow.implicitHeight + Kirigami.Units.smallSpacing
                Rectangle {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    height: parent.height
                    radius: Kirigami.Units.cornerRadius
                    width: parent.width * Math.max(0.02, (modelData.count || 0) / topCard.most)
                    color: Qt.alpha(topCard.barColor, 0.14)
                }
                RowLayout {
                    id: entryRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: Kirigami.Units.smallSpacing * 1.5
                    anchors.rightMargin: Kirigami.Units.smallSpacing * 1.5
                    PlasmaComponents.Label {
                        text: modelData.name
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    PlasmaComponents.Label {
                        text: (modelData.count || 0).toLocaleString(Qt.locale(), "f", 0)
                        font.features: { "tnum": 1 }
                        opacity: 0.75
                    }
                }
            }
        }
    }

    PlasmaExtras.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: dnsTab.dns === null
        iconName: "security-low"
        text: i18n("AdGuard Home is unreachable")
        explanation: i18n("Check the URL and credentials in ~/.config/Linux-Router-Monitor/config.json")
    }

    PopScroll {
        anchors.fill: parent
        visible: dnsTab.dns !== null

        PopCard {
            title: i18n("AdGuard Home")
            icon: "security-high"

            RowLayout {
                Layout.fillWidth: true
                visible: Plasmoid.configuration.showProtection
                Kirigami.Icon {
                    source: dnsTab.dns && dnsTab.dns.protection ? "security-high" : "security-low"
                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                }
                ColumnLayout {
                    spacing: 0
                    Layout.fillWidth: true
                    PlasmaComponents.Label {
                        text: i18n("Protection")
                        font.weight: Font.DemiBold
                    }
                    PlasmaComponents.Label {
                        text: dnsTab.dns && dnsTab.dns.protection ? i18n("Blocking ads and trackers") : i18n("Everything is allowed through")
                        font: Kirigami.Theme.smallFont
                        opacity: 0.65
                    }
                }
                PlasmaComponents.Switch {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    Layout.maximumWidth: implicitIndicatorWidth + leftPadding + rightPadding
                    checked: dnsTab.dns ? dnsTab.dns.protection === true : false
                    onToggled: root.ctlRun("protection " + (checked ? "on" : "off"))
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing * 2

                PopRing {
                    value: dnsTab.dns ? dnsTab.dns.blocked_pct || 0 : 0
                    text: dnsTab.dns ? (dnsTab.dns.blocked_pct || 0).toFixed(1) : ""
                    label: i18n("Blocked")
                    color: root.accent
                    diameter: Kirigami.Units.gridUnit * 5.4
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: Kirigami.Units.largeSpacing
                    rowSpacing: Kirigami.Units.largeSpacing
                    PopStat {
                        Layout.fillWidth: true
                        label: i18n("Queries")
                        value: dnsTab.dns ? (dnsTab.dns.queries_total || 0).toLocaleString(Qt.locale(), "f", 0) : ""
                    }
                    PopStat {
                        Layout.fillWidth: true
                        label: i18n("Blocked")
                        value: dnsTab.dns ? (dnsTab.dns.blocked_total || 0).toLocaleString(Qt.locale(), "f", 0) : ""
                        color: root.accent
                    }
                    PopStat {
                        Layout.fillWidth: true
                        label: i18n("Rate")
                        value: dnsTab.dns ? (dnsTab.dns.qps || 0).toFixed(1) : ""
                        unit: i18n("q/s")
                    }
                    PopStat {
                        Layout.fillWidth: true
                        label: i18n("Response")
                        value: dnsTab.dns ? Math.round(dnsTab.dns.avg_ms || 0) + "" : ""
                        unit: "ms"
                        color: Style.heatStrong(dnsTab.dns ? dnsTab.dns.avg_ms || 0 : 0, 50, 100, Kirigami.Theme)
                    }
                    PopStat {
                        Layout.fillWidth: true
                        label: i18n("Malware")
                        value: dnsTab.dns ? (dnsTab.dns.malware || 0) + "" : ""
                    }
                    PopStat {
                        Layout.fillWidth: true
                        label: i18n("Parental")
                        value: dnsTab.dns ? (dnsTab.dns.parental || 0) + "" : ""
                    }
                }
            }
        }

        PopCard {
            visible: Plasmoid.configuration.showChart && dnsTab.dns !== null && (dnsTab.dns.history || []).length > 0
            title: i18n("Queries over time")
            icon: "view-statistics"
            trailing: dnsTab.dns && dnsTab.dns.time_units === "days" ? i18n("per day") : i18n("per hour")

            Sparkline {
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * 4
                values: dnsTab.dns ? dnsTab.dns.history || [] : []
                values2: dnsTab.dns ? dnsTab.dns.history_blocked || [] : []
                lineColor: Style.hue("down", Kirigami.Theme)
                lineColor2: Kirigami.Theme.negativeTextColor
                gradient: false
                tipText: function(v, index, total) {
                    const unit = dnsTab.dns && dnsTab.dns.time_units === "days" ? i18n("d") : i18n("h")
                    const ago = total - 1 - index
                    return i18n("%1 queries · %2", Math.round(v), ago === 0 ? i18n("now") : ago + unit + i18n(" ago"))
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                Repeater {
                    model: [
                        { label: i18n("All queries"), color: Style.hue("down", Kirigami.Theme) },
                        { label: i18n("Blocked"), color: Kirigami.Theme.negativeTextColor }
                    ]
                    RowLayout {
                        required property var modelData
                        spacing: Kirigami.Units.smallSpacing
                        Rectangle {
                            width: Kirigami.Units.gridUnit * 0.7
                            height: 3
                            radius: 1.5
                            color: modelData.color
                        }
                        PlasmaComponents.Label {
                            text: modelData.label
                            font: Kirigami.Theme.smallFont
                            opacity: 0.7
                        }
                    }
                }
            }
        }

        TopList {
            visible: Plasmoid.configuration.showTopLists && entries.length > 0
            title: i18n("Top blocked")
            icon: "dialog-cancel"
            entries: dnsTab.dns ? dnsTab.dns.top_blocked || [] : []
            barColor: Kirigami.Theme.negativeTextColor
        }
        TopList {
            visible: Plasmoid.configuration.showTopLists && entries.length > 0
            title: i18n("Top queried")
            icon: "internet-web-browser"
            entries: dnsTab.dns ? dnsTab.dns.top_queried || [] : []
        }
        TopList {
            visible: Plasmoid.configuration.showTopLists && entries.length > 0
            title: i18n("Top clients")
            icon: "computer"
            entries: dnsTab.dns ? dnsTab.dns.top_clients || [] : []
            barColor: Style.hue("memory", Kirigami.Theme)
        }
    }
}
