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
    readonly property string chipStyle: Plasmoid.configuration.compactStyle
    readonly property bool showLabels: vertical || thickness >= Kirigami.Units.gridUnit * 2.4
    readonly property bool temps: Plasmoid.configuration.compactShowTemps
    readonly property string lockedStage: Plasmoid.configuration.panelDetail === "auto" ? "" : Plasmoid.configuration.panelDetail
    readonly property bool hasData: !!root.snap.cpu
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
        else
            root.expanded = !wasExpanded
    }

    PopFit {
        id: fit
        vertical: compact.vertical
        thickness: compact.thickness
        span: compact.vertical ? compact.height : compact.width
        spacing: compact.vertical ? chips.rowSpacing : chips.columnSpacing
        shrink: Plasmoid.configuration.panelShrink
        flushEnds: true
        fixed: compact.hasData ? 0 : Math.round(compact.thickness * 0.55)
        items: [cpuChip.stageSpan, gpuChip.stageSpan, ramChip.stageSpan]
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
    }

    Kirigami.Icon {
        anchors.centerIn: parent
        visible: !compact.hasData
        width: Math.round(compact.thickness * 0.55)
        height: width
        source: root.panelIcon
        opacity: 0.6
    }

    GridLayout {
        id: chips
        anchors.centerIn: parent
        visible: compact.hasData
        flow: compact.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: Kirigami.Units.smallSpacing

        PopChip {
            id: cpuChip
            visible: Plasmoid.configuration.compactShowCpu
            vertical: compact.vertical
            adaptive: true
            panelThickness: fit.innerThickness
            chipStyle: compact.chipStyle
            showLabel: compact.showLabels
            minimumStage: "small"
            lockedStage: compact.lockedStage
            fitSpace: fit.space(0)
            label: i18n("CPU")
            widestValue: "100%"
            widestSecondary: compact.temps ? "100°" : ""
            value: Math.round(root.cpu.total || 0) + "%"
            valueColor: Style.heat(root.cpu.total || 0, 85, 95, Kirigami.Theme)
            secondary: compact.temps && root.cpu.temp ? Math.round(root.cpu.temp) + "°" : ""
            secondaryColor: Style.heat(root.cpu.temp || 0, 80, 95, Kirigami.Theme)
            fraction: (root.cpu.total || 0) / 100
            barColor: root.accent
        }
        PopChip {
            id: gpuChip
            visible: Plasmoid.configuration.compactShowGpu && root.gpu !== null
            vertical: compact.vertical
            adaptive: true
            panelThickness: fit.innerThickness
            chipStyle: compact.chipStyle
            showLabel: compact.showLabels
            lockedStage: compact.lockedStage
            fitSpace: fit.space(1)
            label: i18n("GPU")
            widestValue: "100%"
            widestSecondary: compact.temps ? "100°" : ""
            value: root.gpu ? Math.round(root.gpu.util || 0) + "%" : ""
            valueColor: Style.heat(root.gpu ? root.gpu.util || 0 : 0, 90, 99, Kirigami.Theme)
            secondary: compact.temps && root.gpu && root.gpu.temp ? Math.round(root.gpu.temp) + "°" : ""
            secondaryColor: Style.heat(root.gpu ? root.gpu.temp || 0 : 0, 75, 88, Kirigami.Theme)
            fraction: root.gpu ? (root.gpu.util || 0) / 100 : 0
            barColor: Style.hue("up", Kirigami.Theme)
        }
        PopChip {
            id: ramChip
            visible: Plasmoid.configuration.compactShowRam
            vertical: compact.vertical
            adaptive: true
            panelThickness: fit.innerThickness
            chipStyle: compact.chipStyle
            showLabel: compact.showLabels
            lockedStage: compact.lockedStage
            fitSpace: fit.space(2)
            label: i18n("RAM")
            widestValue: "100%"
            value: Math.round(root.mem.pct || 0) + "%"
            valueColor: Style.heat(root.mem.pct || 0, 75, 90, Kirigami.Theme)
            fraction: (root.mem.pct || 0) / 100
            barColor: Style.hue("memory", Kirigami.Theme)
        }
    }
}
