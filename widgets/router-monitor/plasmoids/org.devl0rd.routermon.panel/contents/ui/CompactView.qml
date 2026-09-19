import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import "lib"
import "lib/PopStyle.js" as Style

MouseArea {
    id: compact

    property bool overlayMode: false
    property color overlayBackgroundColor: Qt.rgba(25 / 255, 25 / 255, 25 / 255, 1)
    property real overlayBackgroundOpacity: 1
    signal overlayClicked()
    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property real overlayPreferredWidth: overlayMode ? fit.preferredSpan : Layout.preferredWidth
    readonly property real thickness: vertical ? width : height
    readonly property bool showLabels: vertical || thickness >= Kirigami.Units.gridUnit * 2.4
    readonly property string extra: Plasmoid.configuration.compactExtra
    readonly property var down: root.speed(root.network.down_mbps)
    readonly property var up: root.speed(root.network.up_mbps)
    readonly property bool showNumbers: root.ready && root.routerState !== "offline"
    readonly property string lockedStage: Plasmoid.configuration.panelDetail === "auto" ? "" : Plasmoid.configuration.panelDetail
    readonly property real downScale: Plasmoid.configuration.maxMbps > 0 ? Plasmoid.configuration.maxMbps
                                    : Plasmoid.configuration.planDownMbps > 0 ? Plasmoid.configuration.planDownMbps
                                    : Math.max(1, Plasmoid.configuration.peakDown)
    readonly property real upScale: Plasmoid.configuration.maxMbps > 0 ? Plasmoid.configuration.maxMbps : Math.max(1, Plasmoid.configuration.peakUp)
    readonly property real screenSpan: vertical ? Screen.height : Screen.width
    readonly property bool wideScreen: Screen.width >= Screen.height
    property bool probingWidth: true
    property real settledWidth: 0
    property bool wasExpanded: false

    onScreenSpanChanged: {
        probingWidth = true
        settledWidth = 0
        widthProbe.restart()
    }

    acceptedButtons: Qt.LeftButton | Qt.MiddleButton
    hoverEnabled: true
    onContainsMouseChanged: root.tooltipWanted = containsMouse
    onPressed: function(mouse) { wasExpanded = root.expanded }
    onClicked: function(mouse) {
        if (mouse.button === Qt.MiddleButton)
            root.middleClick()
        else if (overlayMode)
            compact.overlayClicked()
        else
            root.expanded = !wasExpanded
    }

    PopFit {
        id: fit
        vertical: compact.vertical
        thickness: compact.thickness
        span: compact.vertical ? compact.height : compact.width
        spacing: compact.vertical ? content.rowSpacing : content.columnSpacing
        shrink: Plasmoid.configuration.panelShrink
        flushEnds: true
        fixed: compact.showNumbers ? 0 : Math.round(compact.thickness * 0.6)
        items: [downChip.stageSpan, upChip.stageSpan, pingChip.stageSpan, clientsChip.stageSpan, blockedChip.stageSpan]
    }

    Timer {
        id: widthProbe
        interval: 200
        running: true
        onTriggered: {
            compact.settledWidth = fit.tileSpan
            compact.probingWidth = false
        }
    }

    Layout.minimumWidth: vertical ? 0 : (fit.shrink && !wideScreen ? fit.minimumSpan : fit.preferredSpan)
    Layout.preferredWidth: vertical ? 0 : (fit.shrink && !probingWidth && settledWidth > 0 ? settledWidth : fit.preferredSpan)
    Layout.minimumHeight: vertical ? (fit.shrink ? fit.minimumSpan : fit.preferredSpan) : 0
    Layout.preferredHeight: vertical ? fit.preferredSpan : 0

    PopTile {
        vertical: compact.vertical
        inset: fit.inset
        endInset: fit.endInset
        span: fit.tileSpan
        lit: compact.containsMouse || root.expanded
        customBackground: compact.overlayMode
        backgroundColor: compact.overlayBackgroundColor
        backgroundOpacity: compact.overlayBackgroundOpacity
    }

    Item {
        anchors.centerIn: parent
        visible: !compact.showNumbers
        width: Math.round(compact.thickness * 0.6)
        height: width
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

    GridLayout {
        id: content
        anchors.centerIn: parent
        visible: compact.showNumbers
        flow: compact.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: Kirigami.Units.smallSpacing

        PopChip {
            id: downChip
            minimumStage: "small"
            vertical: compact.vertical
            adaptive: true
            panelThickness: fit.innerThickness
            showLabel: compact.showLabels
            lockedStage: compact.lockedStage
            fitSpace: fit.space(0)
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
            id: upChip
            minimumStage: "small"
            vertical: compact.vertical
            adaptive: true
            panelThickness: fit.innerThickness
            showLabel: compact.showLabels
            lockedStage: compact.lockedStage
            fitSpace: fit.space(1)
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
            id: pingChip
            minimumStage: "small"
            visible: compact.extra === "ping"
            vertical: compact.vertical
            adaptive: true
            panelThickness: fit.innerThickness
            showLabel: compact.showLabels
            lockedStage: compact.lockedStage
            fitSpace: fit.space(2)
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
            id: clientsChip
            minimumStage: "small"
            visible: compact.extra === "clients"
            vertical: compact.vertical
            adaptive: true
            panelThickness: fit.innerThickness
            showLabel: compact.showLabels
            lockedStage: compact.lockedStage
            fitSpace: fit.space(3)
            label: i18n("DEVICES")
            widestValue: "888"
            value: root.onlineCount + ""
            fraction: root.leases.length > 0 ? root.onlineCount / root.leases.length : 0
            barColor: root.accent
        }
        PopChip {
            id: blockedChip
            minimumStage: "small"
            visible: compact.extra === "blocked" && root.dns !== null
            vertical: compact.vertical
            adaptive: true
            panelThickness: fit.innerThickness
            showLabel: compact.showLabels
            lockedStage: compact.lockedStage
            fitSpace: fit.space(4)
            label: i18n("BLOCKED")
            widestValue: "100%"
            value: root.dns ? Math.round(root.dns.blocked_pct || 0) + "%" : ""
            fraction: root.dns ? (root.dns.blocked_pct || 0) / 100 : 0
            barColor: root.accent
        }
    }
}
