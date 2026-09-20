import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "lib"
import "lib/PopStyle.js" as Style

PopCard {
    id: card

    readonly property var proc: root.focusProc
    readonly property var history: root.focusHistory
    readonly property int procPid: proc ? proc.pid : 0

    title: i18n("Focused app")
    icon: "window"
    trailing: proc ? "PID " + proc.pid : ""

    component MetricTile: Rectangle {
        id: tile
        property string label
        property string value
        property string unit
        property color valueColor: Kirigami.Theme.textColor
        property var series: []
        property real rangeMax: 0
        property color lineColor: Kirigami.Theme.highlightColor
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        implicitHeight: Kirigami.Units.gridUnit * 3.4
        radius: Kirigami.Units.cornerRadius * 1.5
        color: Qt.alpha(Kirigami.Theme.textColor, 0.04)
        clip: true
        Sparkline {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.height * 0.55
            values: tile.series
            rangeMax: tile.rangeMax
            lineColor: tile.lineColor
            gradient: false
            peakMarker: false
            opacity: 0.8
            tipText: v => tile.label + " " + Math.round(v)
        }
        PopStat {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: Kirigami.Units.smallSpacing * 1.5
            label: tile.label
            value: tile.value
            unit: tile.unit
            color: tile.valueColor
            scale: 1.2
        }
    }

    RowLayout {
        Layout.fillWidth: true
        visible: card.proc !== null
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            Layout.preferredWidth: Kirigami.Units.iconSizes.large
            Layout.preferredHeight: Kirigami.Units.iconSizes.large
            source: card.proc && card.proc.icon ? card.proc.icon : card.proc ? card.proc.name : ""
            fallback: "application-x-executable"
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Kirigami.Heading {
                level: 3
                text: root.focusName
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                PlasmaComponents.Label {
                    text: card.proc ? card.proc.name : ""
                    font: Kirigami.Theme.smallFont
                    opacity: 0.65
                }
                PlasmaComponents.Label {
                    visible: card.proc !== null && card.proc.parentName !== ""
                    text: "·"
                    font: Kirigami.Theme.smallFont
                    opacity: 0.4
                }
                PlasmaComponents.Label {
                    visible: card.proc !== null && card.proc.parentName !== ""
                    text: card.proc ? i18n("started by <a href=\"#\">%1</a>", card.proc.parentName) : ""
                    textFormat: Text.StyledText
                    linkColor: Kirigami.Theme.highlightColor
                    font: Kirigami.Theme.smallFont
                    opacity: 0.8
                    onLinkActivated: {
                        root.expandedPid = card.proc.ppid
                        root.searchText = String(card.proc.ppid)
                    }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                }
            }
        }
        Rectangle {
            visible: card.proc !== null && card.proc.fps >= 0
            implicitWidth: fpsColumn.implicitWidth + Kirigami.Units.largeSpacing * 2
            implicitHeight: fpsColumn.implicitHeight + Kirigami.Units.smallSpacing * 2
            radius: Kirigami.Units.cornerRadius * 2
            color: Qt.alpha(root.fpsColor(card.proc ? card.proc.fps : 0, Kirigami.Theme), 0.14)
            ColumnLayout {
                id: fpsColumn
                anchors.centerIn: parent
                spacing: 0
                PlasmaComponents.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: card.proc ? card.proc.fps + "" : ""
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.9
                    font.weight: Font.DemiBold
                    font.features: { "tnum": 1 }
                    color: root.fpsColor(card.proc ? card.proc.fps : 0, Kirigami.Theme)
                }
                PlasmaComponents.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: i18n("FPS")
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    font.weight: Font.DemiBold
                    opacity: 0.7
                }
            }
        }
    }

    GridLayout {
        Layout.fillWidth: true
        visible: card.proc !== null
        columns: 3
        columnSpacing: Kirigami.Units.smallSpacing * 1.5
        rowSpacing: Kirigami.Units.smallSpacing * 1.5

        MetricTile {
            label: i18n("CPU")
            value: card.proc ? Math.round(card.proc.cpu) + "" : ""
            unit: "%"
            valueColor: root.heatColor(card.proc ? card.proc.cpu : 0, Kirigami.Theme)
            series: card.history.cpu || []
            rangeMax: 100
        }
        MetricTile {
            label: i18n("GPU")
            value: card.proc ? Math.round(card.proc.gpu) + "" : ""
            unit: "%"
            valueColor: root.heatColor(card.proc ? card.proc.gpu : 0, Kirigami.Theme)
            series: card.history.gpu || []
            rangeMax: 100
            lineColor: Style.hue("up", Kirigami.Theme)
        }
        MetricTile {
            visible: card.proc !== null && card.proc.fps >= 0
            label: i18n("Frame time")
            value: card.proc ? card.proc.frametime.toFixed(1) : ""
            unit: i18n("ms")
            series: card.history.frametime || []
            lineColor: Style.hue("down", Kirigami.Theme)
        }
        MetricTile {
            visible: card.proc !== null && card.proc.fps >= 0
            label: i18n("1% low")
            value: card.proc ? card.proc.fpsLow + "" : ""
            unit: i18n("FPS")
            valueColor: root.fpsColor(card.proc ? card.proc.fpsLow : 0, Kirigami.Theme)
            lineColor: Style.hue("down", Kirigami.Theme)
        }
        MetricTile {
            label: i18n("RAM")
            value: card.proc ? Style.bytes(card.proc.ram) : ""
            series: card.history.ram || []
            rangeMax: root.summary.memTotal || 0
            lineColor: Style.hue("memory", Kirigami.Theme)
        }
        MetricTile {
            visible: card.proc !== null && (card.proc.vram > 0 || root.summary.vramTotal > 0)
            label: i18n("VRAM")
            value: card.proc ? (card.proc.vram > 0 ? Style.bytes(card.proc.vram) : "—") : ""
            series: card.history.vram || []
            rangeMax: root.summary.vramTotal || 0
            lineColor: Style.hue("memory", Kirigami.Theme)
        }
        MetricTile {
            label: i18n("Disk")
            value: card.proc ? (card.proc.disk > 0 ? Style.bytes(card.proc.disk) : "—") : ""
            unit: card.proc && card.proc.disk > 0 ? "/s" : ""
            series: card.history.disk || []
            lineColor: Style.hue("fan", Kirigami.Theme)
        }
        MetricTile {
            label: i18n("Threads")
            value: card.proc ? card.proc.threads + "" : ""
            series: card.history.threads || []
        }
        MetricTile {
            visible: card.proc !== null && (card.proc.enc > 0 || card.proc.dec > 0)
            label: i18n("Encode · decode")
            value: card.proc ? Math.round(card.proc.enc) + " · " + Math.round(card.proc.dec) : ""
            unit: "%"
            series: card.history.enc || []
            rangeMax: 100
        }
    }

    PopActions {
        visible: card.proc !== null
        showText: true
        model: card.procPid > 0 ? [
            { icon: "process-stop", text: i18n("End"), run: () => root.signalProc(card.procPid, "TERM") },
            { icon: "process-stop", text: i18n("Force kill"), destructive: true, run: () => root.signalProc(card.procPid, "KILL") },
            { icon: "media-playback-pause", text: i18n("Pause"), run: () => root.signalProc(card.procPid, "STOP") },
            { icon: "media-playback-start", text: i18n("Resume"), run: () => root.signalProc(card.procPid, "CONT") },
            { icon: "folder-open", text: i18n("Open location"), run: () => root.openLocation(card.procPid) },
            { icon: "edit-copy", text: i18n("Copy command"), run: () => root.copyCmdline(card.procPid) }
        ] : []
    }

    PlasmaComponents.Label {
        visible: card.proc === null
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: i18n("No app is focused")
        opacity: 0.6
    }
}
