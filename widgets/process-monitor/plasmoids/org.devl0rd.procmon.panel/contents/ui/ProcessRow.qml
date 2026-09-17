import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "lib"
import "lib/PopStyle.js" as Style
import "lib/Highlight.js" as Highlight

Rectangle {
    id: row

    required property int index
    required property int pid
    required property int depth
    required property bool hasChildren
    required property bool expanded
    readonly property var proc: root.procByPid[pid] || ({})
    readonly property bool open: root.expandedPid === pid
    readonly property real rowHeight: Kirigami.Units.gridUnit * 1.75
    readonly property string query: root.searchText.trim()
    property string commandLine
    signal menuRequested(var proc)

    implicitHeight: rowHeight + (open && details.item ? details.item.implicitHeight + Kirigami.Units.smallSpacing * 2 : 0)
    radius: Kirigami.Units.cornerRadius
    color: open ? Qt.alpha(Kirigami.Theme.highlightColor, 0.1)
         : mouse.containsMouse ? Qt.alpha(Kirigami.Theme.highlightColor, 0.12)
         : index % 2 ? Qt.alpha(Kirigami.Theme.textColor, 0.03) : "transparent"
    border.width: open ? 1 : 0
    border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.4)
    clip: true
    Behavior on implicitHeight { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

    onOpenChanged: {
        if (open)
            root.readCommand(root.commandLineCommand(pid), text => row.commandLine = text)
    }

    MouseArea {
        id: mouse
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: row.rowHeight
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function(event) {
            if (event.button === Qt.RightButton)
                row.menuRequested(row.proc)
            else
                root.expandedPid = row.open ? 0 : row.pid
        }
    }

    Sparkline {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: row.rowHeight - 2
        visible: Plasmoid.configuration.sortColumn !== "name" && Plasmoid.configuration.sortColumn !== "pid"
        values: root.sortHistByPid[row.pid] || []
        rangeMax: root.graphMax(Plasmoid.configuration.sortColumn)
        lineColor: Kirigami.Theme.highlightColor
        gradient: false
        peakMarker: false
        opacity: 0.3
        hoverable: false
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: row.rowHeight
        anchors.leftMargin: Kirigami.Units.smallSpacing
        anchors.rightMargin: Kirigami.Units.smallSpacing
        spacing: Kirigami.Units.smallSpacing

        Item {
            Layout.preferredWidth: row.depth * Kirigami.Units.gridUnit
        }
        PlasmaComponents.ToolButton {
            visible: row.hasChildren
            Layout.preferredWidth: Kirigami.Units.gridUnit * 1.3
            Layout.preferredHeight: Kirigami.Units.gridUnit * 1.3
            icon.name: row.expanded ? "arrow-down" : "arrow-right"
            onClicked: root.toggleTree(row.pid)
        }
        Item {
            visible: !row.hasChildren && Plasmoid.configuration.treeView && root.searchText === "" && Plasmoid.configuration.processFilter === "all"
            Layout.preferredWidth: Kirigami.Units.gridUnit * 1.3
        }
        Kirigami.Icon {
            Layout.preferredWidth: Kirigami.Units.iconSizes.small
            Layout.preferredHeight: Kirigami.Units.iconSizes.small
            source: row.proc.icon ? row.proc.icon : (row.proc.name || "application-x-executable")
            fallback: "application-x-executable"
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            text: row.query === "" ? row.proc.name || "" : Highlight.mark(row.proc.name || "", row.query, Kirigami.Theme.highlightColor)
            textFormat: row.query === "" ? Text.PlainText : Text.StyledText
            elide: Text.ElideRight
        }
        Rectangle {
            visible: row.proc.fps !== undefined
            implicitWidth: fpsLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
            implicitHeight: fpsLabel.implicitHeight
            radius: height / 2
            color: Qt.alpha(root.fpsColor(row.proc.fps || 0), 0.16)
            PlasmaComponents.Label {
                id: fpsLabel
                anchors.centerIn: parent
                text: (row.proc.fps || 0) + " FPS"
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                font.weight: Font.DemiBold
                color: root.fpsColor(row.proc.fps || 0)
            }
        }
        Repeater {
            model: root.columns
            PlasmaComponents.Label {
                required property var modelData
                Layout.preferredWidth: Kirigami.Units.gridUnit * 3.6
                horizontalAlignment: Text.AlignRight
                text: modelData.key === "pid" && row.query !== "" ? Highlight.mark(String(row.pid), row.query, Kirigami.Theme.highlightColor) : root.fmtCol(row.proc, modelData)
                textFormat: modelData.key === "pid" && row.query !== "" ? Text.StyledText : Text.PlainText
                color: root.colColor(row.proc, modelData)
                font.features: { "tnum": 1 }
                opacity: modelData.kind === "bytes" || modelData.kind === "int" ? 0.9 : 1
                elide: Text.ElideRight
            }
        }
    }

    component MetricLine: RowLayout {
        property string label
        property string own
        property string total
        property bool header: false
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing
        PlasmaComponents.Label {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 4
            text: parent.label
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            opacity: 0.6
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            text: parent.own
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            font.weight: parent.header ? Font.DemiBold : Font.Normal
            font.features: { "tnum": 1 }
            opacity: parent.header ? 0.6 : 0.9
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            text: parent.total
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            font.weight: parent.header ? Font.DemiBold : Font.Normal
            font.features: { "tnum": 1 }
            opacity: parent.header ? 0.6 : 0.9
        }
    }

    Loader {
        id: details
        active: row.open
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: row.rowHeight
        anchors.leftMargin: Kirigami.Units.largeSpacing
        anchors.rightMargin: Kirigami.Units.largeSpacing
        sourceComponent: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                MetricLine { header: true; own: i18n("This process"); total: i18n("With children") }
                Repeater {
                    model: [
                        { key: "cpu", label: i18n("CPU"), kind: "pct" },
                        { key: "gpu", label: i18n("GPU"), kind: "pct" },
                        { key: "ram", label: i18n("RAM"), kind: "bytes" },
                        { key: "vram", label: i18n("VRAM"), kind: "bytes" },
                        { key: "disk", label: i18n("Disk"), kind: "rate" },
                        { key: "threads", label: i18n("Threads"), kind: "int" },
                        { key: "enc", label: i18n("Encode"), kind: "pct" },
                        { key: "dec", label: i18n("Decode"), kind: "pct" }
                    ]
                    MetricLine {
                        required property var modelData
                        label: modelData.label
                        own: root.fmtValue(row.proc[modelData.key] || 0, modelData.kind)
                        total: root.fmtValue(row.proc["a" + modelData.key] !== undefined ? row.proc["a" + modelData.key] : (row.proc[modelData.key] || 0), modelData.kind)
                    }
                }
            }

            RowLayout {
                visible: row.proc.fps !== undefined
                Layout.fillWidth: true
                PlasmaComponents.Label {
                    text: i18n("Frames")
                    font: Kirigami.Theme.smallFont
                    opacity: 0.6
                }
                PlasmaComponents.Label {
                    text: i18n("%1 FPS · %2 ms per frame · 1% low %3 FPS", row.proc.fps || 0, (row.proc.frametime || 0).toFixed(1), row.proc.fps_low || 0)
                    font: Kirigami.Theme.smallFont
                    color: root.fpsColor(row.proc.fps || 0)
                }
            }

            PlasmaComponents.Label {
                Layout.fillWidth: true
                visible: row.commandLine !== ""
                text: row.commandLine
                font.family: "monospace"
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                opacity: 0.75
                wrapMode: Text.WrapAnywhere
                maximumLineCount: 3
                elide: Text.ElideRight
            }

            PopActions {
                showText: true
                model: [
                    { icon: "process-stop", text: i18n("End"), run: () => root.signalProc(row.pid, "TERM") },
                    { icon: "process-stop", text: i18n("Force kill"), destructive: true, run: () => root.signalProc(row.pid, "KILL") },
                    { icon: "media-playback-pause", text: i18n("Pause"), run: () => root.signalProc(row.pid, "STOP") },
                    { icon: "media-playback-start", text: i18n("Resume"), run: () => root.signalProc(row.pid, "CONT") },
                    { icon: "folder-open", text: i18n("Open location"), run: () => root.openLocation(row.pid) },
                    { icon: "edit-copy", text: i18n("Copy command"), run: () => root.copyText(row.commandLine) },
                    { icon: "overflow-menu", text: i18n("More…"), run: () => row.menuRequested(row.proc) }
                ]
            }
        }
    }
}
