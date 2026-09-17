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
    readonly property var proc: root.focusProc
    readonly property bool system: proc === null
    readonly property real cpu: system ? root.summary.cpu : proc.cpu
    readonly property real gpu: system ? root.summary.gpu : proc.gpu
    readonly property bool showFps: Plasmoid.configuration.compactShowFps && proc !== null && proc.fps >= 0
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
    readonly property real sidePadding: Kirigami.Units.largeSpacing * 1.25
    readonly property bool lit: containsMouse || root.expanded

    Layout.minimumWidth: vertical ? 0 : content.implicitWidth + sidePadding * 2 + inset * 2
    Layout.preferredWidth: Layout.minimumWidth
    Layout.minimumHeight: vertical ? content.implicitHeight + sidePadding * 2 + inset * 2 : 0
    Layout.preferredHeight: Layout.minimumHeight

    Rectangle {
        id: tile
        anchors.fill: parent
        anchors.margins: compact.inset
        radius: Kirigami.Units.cornerRadius * 2
        border.width: 1
        border.color: Qt.alpha(Kirigami.Theme.textColor, compact.lit ? 0.28 : 0.16)
        gradient: Gradient {
            orientation: compact.vertical ? Gradient.Horizontal : Gradient.Vertical
            GradientStop { position: 0; color: Qt.alpha(Kirigami.Theme.textColor, compact.lit ? 0.16 : 0.10) }
            GradientStop { position: 1; color: Qt.alpha(Kirigami.Theme.textColor, compact.lit ? 0.09 : 0.04) }
        }
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
            Layout.preferredWidth: Math.round(compact.valueSize * 1.35)
            Layout.preferredHeight: Layout.preferredWidth
            source: compact.system ? root.panelIcon
                  : root.activeIcon ? root.activeIcon
                  : compact.proc.icon ? compact.proc.icon : compact.proc.name
            fallback: root.panelIcon
        }

        PlasmaComponents.Label {
            id: nameLabel
            visible: !compact.vertical && Plasmoid.configuration.compactMaxWidth > 0
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: Kirigami.Units.gridUnit * Plasmoid.configuration.compactMaxWidth
            Layout.maximumWidth: Layout.preferredWidth
            text: compact.system ? i18n("System") : root.focusName
            opacity: compact.system ? 0.7 : 1
            font.pixelSize: compact.valueSize * 0.92
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        PopChip {
            visible: Plasmoid.configuration.compactShowCpu
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: "text"
            label: i18n("CPU")
            widestValue: "100%"
            value: Math.round(compact.cpu) + "%"
            valueColor: Style.heat(compact.cpu, 60, 85, Kirigami.Theme)
        }
        PopChip {
            visible: Plasmoid.configuration.compactShowGpu
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: "text"
            label: i18n("GPU")
            widestValue: "100%"
            value: Math.round(compact.gpu) + "%"
            valueColor: Style.heat(compact.gpu, 90, 99, Kirigami.Theme)
        }
        PopChip {
            visible: compact.showFps
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: "text"
            label: i18n("FPS")
            widestValue: "888"
            value: compact.proc ? compact.proc.fps + "" : ""
            valueColor: root.fpsColor(compact.proc ? compact.proc.fps : 0)
            showLabel: compact.vertical
        }
    }
}
