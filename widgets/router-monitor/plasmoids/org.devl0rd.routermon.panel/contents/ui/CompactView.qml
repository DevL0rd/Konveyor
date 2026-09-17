import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import "lib"
import "lib/PopStyle.js" as Style

MouseArea {
    id: compact

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property real thickness: vertical ? width : height
    readonly property bool showLabels: vertical || thickness >= Kirigami.Units.gridUnit * 2.4
    readonly property string extra: Plasmoid.configuration.compactExtra
    readonly property var down: root.speed(root.network.down_mbps)
    readonly property var up: root.speed(root.network.up_mbps)
    readonly property bool showNumbers: root.ready && root.routerState !== "offline"
    readonly property real downScale: Plasmoid.configuration.maxMbps > 0 ? Plasmoid.configuration.maxMbps
                                    : Plasmoid.configuration.planDownMbps > 0 ? Plasmoid.configuration.planDownMbps
                                    : Math.max(1, Plasmoid.configuration.peakDown)
    readonly property real upScale: Plasmoid.configuration.maxMbps > 0 ? Plasmoid.configuration.maxMbps : Math.max(1, Plasmoid.configuration.peakUp)
    property bool wasExpanded: false

    acceptedButtons: Qt.LeftButton | Qt.MiddleButton
    hoverEnabled: true
    onContainsMouseChanged: root.tooltipWanted = containsMouse
    onPressed: function(mouse) { wasExpanded = root.expanded }
    onClicked: function(mouse) {
        if (mouse.button === Qt.MiddleButton)
            root.middleClick()
        else
            root.expanded = !wasExpanded
    }

    Layout.minimumWidth: vertical ? 0 : content.implicitWidth + Kirigami.Units.smallSpacing * 2
    Layout.preferredWidth: Layout.minimumWidth
    Layout.minimumHeight: vertical ? content.implicitHeight + Kirigami.Units.smallSpacing * 2 : 0
    Layout.preferredHeight: Layout.minimumHeight

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.textColor, compact.containsMouse || root.expanded ? 0.08 : 0)
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    GridLayout {
        id: content
        anchors.centerIn: parent
        flow: compact.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: Kirigami.Units.smallSpacing

        Item {
            visible: !compact.showNumbers
            Layout.alignment: Qt.AlignCenter
            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
            Layout.preferredHeight: Layout.preferredWidth
            Kirigami.Icon {
                anchors.fill: parent
                source: root.routerState === "offline" ? "network-disconnect" : "network-wireless-hotspot"
                opacity: root.routerState === "offline" ? 0.7 : 1
            }
            Rectangle {
                visible: root.routerState !== "ok"
                width: Math.round(parent.width * 0.5)
                height: width
                radius: width / 2
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: -2
                color: root.stateColor
                border.width: 1.5
                border.color: Kirigami.Theme.backgroundColor
                Kirigami.Icon {
                    visible: root.routerState === "paused"
                    anchors.fill: parent
                    anchors.margins: 1
                    source: "media-playback-pause"
                    color: Kirigami.Theme.backgroundColor
                    isMask: true
                }
            }
        }

        PopChip {
            visible: compact.showNumbers
            vertical: compact.vertical
            panelThickness: compact.thickness
            showLabel: compact.showLabels
            label: i18n("DOWN")
            valueColor: root.routerState === "ok" ? Kirigami.Theme.textColor : root.stateColor
            widestValue: "888.8"
            widestSecondary: compact.vertical ? "" : i18n("Mb/s")
            value: compact.down.value
            secondary: compact.vertical ? "" : compact.down.unit
            fraction: Math.min(1, (root.network.down_mbps || 0) / compact.downScale)
            barColor: root.downColor
        }
        PopChip {
            visible: compact.showNumbers
            vertical: compact.vertical
            panelThickness: compact.thickness
            showLabel: compact.showLabels
            label: i18n("UP")
            valueColor: root.routerState === "ok" ? Kirigami.Theme.textColor : root.stateColor
            widestValue: "888.8"
            widestSecondary: compact.vertical ? "" : i18n("Mb/s")
            value: compact.up.value
            secondary: compact.vertical ? "" : compact.up.unit
            fraction: Math.min(1, (root.network.up_mbps || 0) / compact.upScale)
            barColor: root.upColor
        }
        PopChip {
            visible: compact.showNumbers && compact.extra === "ping"
            vertical: compact.vertical
            panelThickness: compact.thickness
            showLabel: compact.showLabels
            label: i18n("PING")
            widestValue: "888"
            widestSecondary: compact.vertical ? "" : "ms"
            value: Math.round(root.network.ping_rtt || 0) + ""
            valueColor: Style.heat(root.network.ping_rtt || 0, 40, 100, Kirigami.Theme)
            secondary: compact.vertical ? "" : "ms"
            fraction: Math.min(1, (root.network.ping_rtt || 0) / 100)
            barColor: Style.heat(root.network.ping_rtt || 0, 40, 100, Kirigami.Theme)
        }
        PopChip {
            visible: compact.showNumbers && compact.extra === "clients"
            vertical: compact.vertical
            panelThickness: compact.thickness
            showLabel: compact.showLabels
            label: i18n("DEVICES")
            widestValue: "888"
            value: root.onlineCount + ""
            fraction: root.leases.length > 0 ? root.onlineCount / root.leases.length : 0
            barColor: root.accent
        }
        PopChip {
            visible: compact.showNumbers && compact.extra === "blocked" && root.dns !== null
            vertical: compact.vertical
            panelThickness: compact.thickness
            showLabel: compact.showLabels
            label: i18n("BLOCKED")
            widestValue: "100%"
            value: root.dns ? Math.round(root.dns.blocked_pct || 0) + "%" : ""
            fraction: root.dns ? (root.dns.blocked_pct || 0) / 100 : 0
            barColor: root.accent
        }
    }
}
