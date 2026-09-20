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

    property int overlayTargetPid: 0
    property bool overlayMode: false
    property color overlayBackgroundColor: Qt.rgba(25 / 255, 25 / 255, 25 / 255, 1)
    property real overlayBackgroundOpacity: 1
    signal overlayClicked()
    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property real overlayPreferredWidth: overlayMode ? fit.preferredSpan : Layout.preferredWidth
    readonly property real thickness: vertical ? width : height
    readonly property real valueSize: Math.max(Kirigami.Units.gridUnit * 0.6, Math.min(Kirigami.Theme.defaultFont.pixelSize * 1.3, fit.innerThickness * 0.52))
    readonly property var proc: overlayTargetPid > 0 ? (root.overlayProcByPid[overlayTargetPid] || null) : root.focusProc
    readonly property bool system: proc === null
    readonly property real cpu: system ? root.summary.cpu : proc.cpu
    readonly property real gpu: system ? root.summary.gpu : proc.gpu
    readonly property bool showFps: Plasmoid.configuration.compactShowFps && proc !== null && proc.fps >= 0
    readonly property int framePid: proc ? (proc.framePid || proc.pid) : 0
    readonly property var frametimes: root.frametimesFor(framePid)
    readonly property string lockedStage: Plasmoid.configuration.panelDetail === "auto" ? "" : Plasmoid.configuration.panelDetail
    readonly property bool lit: containsMouse || root.expanded
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

    readonly property real iconLen: Math.round(fit.innerThickness * 0.8)
    readonly property bool nameWanted: !vertical && Plasmoid.configuration.compactMaxWidth > 0
    readonly property real nameMax: nameWanted ? Kirigami.Units.gridUnit * Plasmoid.configuration.compactMaxWidth : 0
    readonly property real nameSize: nameWanted ? Math.min(nameMax, Math.ceil(nameMetrics.advanceWidth) + Kirigami.Units.smallSpacing * 2) : 0
    readonly property var nameSpan: ({ min: 0, max: nameSize, sizes: [0, 0, nameSize, nameSize], low: 0, high: 3 })

    TextMetrics {
        id: nameMetrics
        font.pixelSize: compact.valueSize * 0.92
        font.weight: Font.DemiBold
        text: compact.system ? i18n("System") : (compact.overlayTargetPid > 0 ? compact.proc.name : root.focusName)
    }

    PopFit {
        id: fit
        vertical: compact.vertical
        thickness: compact.thickness
        span: compact.vertical ? compact.height : compact.width
        spacing: compact.vertical ? content.rowSpacing : content.columnSpacing
        shrink: Plasmoid.configuration.panelShrink
        fixed: compact.iconLen
        items: [compact.nameSpan, cpuChip.stageSpan, gpuChip.stageSpan, fpsChip.stageSpan]
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
    Layout.preferredWidth: vertical ? 0 : (fit.shrink && !wideScreen && !probingWidth && settledWidth > 0 ? settledWidth : fit.preferredSpan)
    Layout.minimumHeight: vertical ? (fit.shrink ? fit.minimumSpan : fit.preferredSpan) : 0
    Layout.preferredHeight: vertical ? fit.preferredSpan : 0

    PopTile {
        id: tile
        vertical: compact.vertical
        inset: fit.inset
        span: fit.tileSpan
        lit: compact.lit
        customBackground: compact.overlayMode
        backgroundColor: compact.overlayBackgroundColor
        backgroundOpacity: compact.overlayBackgroundOpacity
    }

    Sparkline {
        anchors.fill: tile
        anchors.margins: 1
        visible: compact.frametimes.length > 1
        values: compact.frametimes
        rangeFloor: 33.333
        lineColor: Kirigami.Theme.highlightColor
        dangerFrom: 16.667
        dangerColor: Kirigami.Theme.negativeTextColor
        gradient: false
        peakMarker: false
        hoverable: false
        opacity: 0.55
    }

    Rectangle {
        visible: !compact.vertical && nameLabel.visible
        width: 1
        height: tile.height * 0.5
        anchors.verticalCenter: tile.verticalCenter
        x: content.x + nameLabel.x + nameLabel.width + content.columnSpacing / 2
        color: Qt.alpha(Kirigami.Theme.textColor, 0.16)
    }

    GridLayout {
        id: content
        anchors.centerIn: parent
        flow: compact.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: Kirigami.Units.smallSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignCenter
            Layout.preferredWidth: compact.iconLen
            Layout.preferredHeight: Layout.preferredWidth
            source: compact.system ? root.panelIcon
                  : compact.proc.icon ? compact.proc.icon : compact.proc.name
            fallback: root.panelIcon
        }

        PlasmaComponents.Label {
            id: nameLabel
            visible: fit.share[0] > 0
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: Math.floor(Math.min(compact.nameSize, fit.share[0]))
            Layout.maximumWidth: Layout.preferredWidth
            text: nameMetrics.text
            opacity: compact.system ? 0.7 : 1
            font.pixelSize: nameMetrics.font.pixelSize
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        PopChip {
            id: cpuChip
            visible: Plasmoid.configuration.compactShowCpu
            vertical: compact.vertical
            adaptive: true
            panelThickness: fit.innerThickness
            chipStyle: "text"
            contentGap: Kirigami.Units.largeSpacing
            lockedStage: compact.lockedStage
            fitSpace: fit.space(1)
            label: i18n("CPU")
            widestValue: "100%"
            value: Math.round(compact.cpu) + "%"
            valueColor: Style.heat(compact.cpu, 60, 85, Kirigami.Theme)
        }
        PopChip {
            id: gpuChip
            visible: Plasmoid.configuration.compactShowGpu
            vertical: compact.vertical
            adaptive: true
            panelThickness: fit.innerThickness
            chipStyle: "text"
            contentGap: Kirigami.Units.largeSpacing
            lockedStage: compact.lockedStage
            fitSpace: fit.space(2)
            label: i18n("GPU")
            widestValue: "100%"
            value: Math.round(compact.gpu) + "%"
            valueColor: Style.heat(compact.gpu, 90, 99, Kirigami.Theme)
        }
        PopChip {
            id: fpsChip
            visible: compact.showFps
            vertical: compact.vertical
            adaptive: true
            panelThickness: fit.innerThickness
            chipStyle: "text"
            contentGap: Kirigami.Units.largeSpacing
            lockedStage: compact.lockedStage
            fitSpace: fit.space(3)
            label: i18n("FPS")
            widestValue: "888"
            value: compact.proc ? compact.proc.fps + "" : ""
            valueColor: root.fpsColor(compact.proc ? compact.proc.fps : 0, Kirigami.Theme)
            secondary: compact.proc ? i18n("1% %1", compact.proc.fpsLow) : ""
            widestSecondary: i18n("1% 888")
            secondaryColor: root.fpsColor(compact.proc ? compact.proc.fpsLow : 0, Kirigami.Theme)
            showLabel: compact.vertical
        }
    }
}
