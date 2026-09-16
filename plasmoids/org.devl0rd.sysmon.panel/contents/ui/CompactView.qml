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
    property bool wasExpanded: false

    acceptedButtons: Qt.LeftButton | Qt.MiddleButton
    hoverEnabled: true
    onPressed: function(mouse) { wasExpanded = root.expanded }
    onClicked: function(mouse) {
        if (mouse.button === Qt.MiddleButton)
            root.middleClick()
        else
            root.expanded = !wasExpanded
    }

    readonly property real inset: Math.max(2, Math.round(thickness * 0.08))
    readonly property real sidePadding: Kirigami.Units.largeSpacing * 1.5
    readonly property bool lit: containsMouse || root.expanded

    Layout.minimumWidth: vertical ? 0 : chips.implicitWidth + sidePadding * 2 + inset * 2
    Layout.preferredWidth: Layout.minimumWidth
    Layout.minimumHeight: vertical ? chips.implicitHeight + sidePadding * 2 + inset * 2 : 0
    Layout.preferredHeight: Layout.minimumHeight

    Rectangle {
        id: tile
        anchors.fill: parent
        anchors.margins: compact.inset
        radius: Kirigami.Units.cornerRadius * 2
        border.width: 1
        border.color: Qt.alpha(root.accent, compact.lit ? 0.55 : 0.32)
        gradient: Gradient {
            orientation: compact.vertical ? Gradient.Vertical : Gradient.Horizontal
            GradientStop { position: 0; color: Qt.alpha(root.accent, compact.lit ? 0.26 : 0.17) }
            GradientStop { position: 1; color: Qt.alpha(root.accent, compact.lit ? 0.12 : 0.06) }
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: parent.radius - 1
            color: "transparent"
            border.width: 1
            border.color: Qt.alpha("white", 0.05)
        }
    }

    Repeater {
        model: compact.vertical ? 0 : chips.visibleChildren.length - 1
        Rectangle {
            required property int index
            readonly property Item before: chips.visibleChildren[index]
            readonly property Item after: chips.visibleChildren[index + 1]
            visible: !!before && !!after
            width: 1
            height: tile.height * 0.5
            anchors.verticalCenter: tile.verticalCenter
            x: before && after ? chips.x + (before.x + before.width + after.x) / 2 : 0
            color: Qt.alpha(Kirigami.Theme.textColor, 0.12)
        }
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
            visible: Plasmoid.configuration.compactShowCpu
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: compact.chipStyle
            showLabel: compact.showLabels
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
            visible: Plasmoid.configuration.compactShowGpu && root.gpu !== null
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: compact.chipStyle
            showLabel: compact.showLabels
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
            visible: Plasmoid.configuration.compactShowRam
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: compact.chipStyle
            showLabel: compact.showLabels
            label: i18n("RAM")
            widestValue: "100%"
            value: Math.round(root.mem.pct || 0) + "%"
            valueColor: Style.heat(root.mem.pct || 0, 75, 90, Kirigami.Theme)
            fraction: (root.mem.pct || 0) / 100
            barColor: Style.hue("memory", Kirigami.Theme)
        }
    }
}
