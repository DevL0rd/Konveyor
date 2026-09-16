import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents
import "lib"
import "lib/PopStyle.js" as Style

MouseArea {
    id: compact

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property real thickness: vertical ? width : height
    readonly property real valueSize: Math.max(Kirigami.Theme.smallFont.pixelSize, Math.min(Kirigami.Theme.defaultFont.pixelSize * 1.05, thickness * 0.4))
    readonly property string extra: Plasmoid.configuration.compactExtra
    readonly property var down: root.speed(root.network.down_mbps)
    readonly property var up: root.speed(root.network.up_mbps)
    readonly property bool showNumbers: root.ready && root.routerState !== "offline"
    property bool wasExpanded: false

    component SpeedGroup: GridLayout {
        property string arrow
        property var speed
        property color tint
        visible: compact.showNumbers
        Layout.alignment: Qt.AlignCenter
        flow: compact.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        columnSpacing: 2
        rowSpacing: 0
        PlasmaComponents.Label {
            text: parent.arrow
            color: parent.tint
            font.pixelSize: compact.valueSize
            font.weight: Font.Bold
            Layout.alignment: compact.vertical ? Qt.AlignHCenter : Qt.AlignBaseline
        }
        PlasmaComponents.Label {
            text: parent.speed.value
            horizontalAlignment: compact.vertical ? Text.AlignHCenter : Text.AlignRight
            Layout.minimumWidth: compact.vertical ? 0 : Math.ceil(numberMetrics.advanceWidth)
            font.pixelSize: compact.valueSize
            font.weight: Font.DemiBold
            font.features: { "tnum": 1 }
            Layout.alignment: compact.vertical ? Qt.AlignHCenter : Qt.AlignBaseline
        }
        PlasmaComponents.Label {
            visible: !compact.vertical
            Layout.minimumWidth: Math.ceil(unitMetrics.advanceWidth)
            text: parent.speed.unit
            font.pixelSize: compact.valueSize * 0.7
            opacity: 0.6
            Layout.alignment: Qt.AlignBaseline
        }
    }

    TextMetrics {
        id: numberMetrics
        font.pixelSize: compact.valueSize
        font.weight: Font.DemiBold
        font.features: { "tnum": 1 }
        text: "888.8"
    }
    TextMetrics {
        id: unitMetrics
        font.pixelSize: compact.valueSize * 0.7
        text: i18n("Mb/s").length >= i18n("Kb/s").length ? i18n("Mb/s") : i18n("Kb/s")
    }
    TextMetrics {
        id: extraMetrics
        font.pixelSize: compact.valueSize * 0.85
        font.features: { "tnum": 1 }
        text: compact.extra === "ping" ? "888 ms" : compact.extra === "blocked" ? "100%" : "888"
    }

    acceptedButtons: Qt.LeftButton | Qt.MiddleButton
    hoverEnabled: true
    onPressed: function(mouse) { wasExpanded = root.expanded }
    onClicked: function(mouse) {
        if (mouse.button === Qt.MiddleButton)
            root.middleClick()
        else
            root.expanded = !wasExpanded
    }

    Layout.minimumWidth: vertical ? 0 : content.implicitWidth + Kirigami.Units.smallSpacing * 3
    Layout.preferredWidth: Layout.minimumWidth
    Layout.minimumHeight: vertical ? content.implicitHeight + Kirigami.Units.smallSpacing * 3 : 0
    Layout.preferredHeight: Layout.minimumHeight

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.textColor, compact.containsMouse || root.expanded ? 0.08 : 0)
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    Sparkline {
        visible: compact.showNumbers && !compact.vertical
        anchors.fill: parent
        anchors.margins: 3
        opacity: 0.35
        values: root.series("down").slice(-40)
        values2: root.series("up").slice(-40)
        lineColor: root.downColor
        lineColor2: root.upColor
        gradient: false
        peakMarker: false
        hoverable: false
        rangeFloor: 2
    }

    GridLayout {
        id: content
        anchors.centerIn: parent
        flow: compact.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        columnSpacing: Kirigami.Units.smallSpacing * 1.5
        rowSpacing: Kirigami.Units.smallSpacing

        Item {
            Layout.alignment: Qt.AlignCenter
            Layout.preferredWidth: Math.round(compact.valueSize * 1.35)
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

        SpeedGroup {
            arrow: "↓"
            speed: compact.down
            tint: root.downColor
        }
        SpeedGroup {
            arrow: "↑"
            speed: compact.up
            tint: root.upColor
        }

        PlasmaComponents.Label {
            visible: compact.showNumbers && compact.extra !== "none" && text !== ""
            Layout.alignment: Qt.AlignCenter
            Layout.leftMargin: compact.vertical ? 0 : Kirigami.Units.smallSpacing
            font.pixelSize: compact.valueSize * 0.85
            font.features: { "tnum": 1 }
            horizontalAlignment: compact.vertical ? Text.AlignHCenter : Text.AlignRight
            Layout.minimumWidth: compact.vertical ? 0 : Math.ceil(extraMetrics.advanceWidth)
            text: compact.extra === "ping" ? Math.round(root.network.ping_rtt || 0) + " ms"
                : compact.extra === "clients" ? root.onlineCount + ""
                : compact.extra === "blocked" && root.dns ? Math.round(root.dns.blocked_pct || 0) + "%"
                : ""
            color: compact.extra === "ping" ? Style.heat(root.network.ping_rtt || 0, 40, 100, Kirigami.Theme) : Kirigami.Theme.textColor
            opacity: 0.85
        }
    }
}
