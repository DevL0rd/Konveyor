import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import "lib"
import "lib/PopStyle.js" as Style
import "lib/PopStage.js" as Stage

MouseArea {
    id: compact

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property real thickness: vertical ? width : height
    readonly property string chipStyle: Plasmoid.configuration.compactStyle
    readonly property bool showLabels: vertical || thickness >= Kirigami.Units.gridUnit * 2.4
    readonly property bool temps: Plasmoid.configuration.compactShowTemps
    readonly property bool shrink: Plasmoid.configuration.panelShrink
    readonly property string lockedStage: Plasmoid.configuration.panelDetail === "auto" ? "" : Plasmoid.configuration.panelDetail
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

    readonly property real padding: Kirigami.Units.smallSpacing * 2
    readonly property real spacing: vertical ? chips.rowSpacing : chips.columnSpacing
    readonly property var chipItems: [cpuChip.stageSpan, ramChip.stageSpan, gpuChip.stageSpan]
    readonly property real chipSpace: (vertical ? height : width) - padding
    readonly property var chipShare: Stage.share(chipSpace, chipItems, spacing)
    readonly property real minimumSpan: Math.ceil(Stage.span(chipItems, spacing, "min")) + padding
    readonly property real preferredSpan: Math.ceil(Stage.span(chipItems, spacing, "max")) + padding

    Layout.minimumWidth: vertical ? 0 : Math.max(thickness, shrink ? minimumSpan : preferredSpan)
    Layout.preferredWidth: vertical ? 0 : Math.max(thickness, preferredSpan)
    Layout.minimumHeight: vertical ? Math.max(thickness, shrink ? minimumSpan : preferredSpan) : 0
    Layout.preferredHeight: vertical ? Math.max(thickness, preferredSpan) : 0

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.textColor, compact.containsMouse || root.expanded ? 0.08 : 0)
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    Kirigami.Icon {
        anchors.centerIn: parent
        visible: !root.snap.cpu
        width: Math.min(parent.width, parent.height) * 0.8
        height: width
        source: root.panelIcon
        opacity: 0.6
    }

    GridLayout {
        id: chips
        anchors.centerIn: parent
        visible: !!root.snap.cpu
        flow: compact.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: Kirigami.Units.smallSpacing

        PopChip {
            id: cpuChip
            visible: Plasmoid.configuration.compactShowCpu
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: compact.chipStyle
            showLabel: compact.showLabels
            minimumStage: "small"
            lockedStage: compact.lockedStage
            fitSpace: compact.shrink ? compact.chipShare[0] : -1
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
            cappedStage: ramChip.stage
            visible: Plasmoid.configuration.compactShowGpu && root.gpu !== null
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: compact.chipStyle
            showLabel: compact.showLabels
            lockedStage: compact.lockedStage
            fitSpace: compact.shrink ? compact.chipShare[2] : -1
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
            cappedStage: cpuChip.stage
            visible: Plasmoid.configuration.compactShowRam
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: compact.chipStyle
            showLabel: compact.showLabels
            lockedStage: compact.lockedStage
            fitSpace: compact.shrink ? compact.chipShare[1] : -1
            label: i18n("RAM")
            widestValue: "100%"
            value: Math.round(root.mem.pct || 0) + "%"
            valueColor: Style.heat(root.mem.pct || 0, 75, 90, Kirigami.Theme)
            fraction: (root.mem.pct || 0) / 100
            barColor: Style.hue("memory", Kirigami.Theme)
        }
    }
}
