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

    GridLayout {
        id: content
        anchors.centerIn: parent
        flow: compact.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        columnSpacing: Kirigami.Units.smallSpacing * 1.5
        rowSpacing: Kirigami.Units.smallSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignCenter
            Layout.preferredWidth: Math.round(compact.valueSize * 1.35)
            Layout.preferredHeight: Layout.preferredWidth
            source: root.activePid > 0 && root.activeIcon ? root.activeIcon
                  : compact.proc && compact.proc.icon ? compact.proc.icon
                  : compact.proc ? compact.proc.name : root.panelIcon
            fallback: root.panelIcon
        }

        PlasmaComponents.Label {
            visible: !compact.vertical && compact.proc !== null && Plasmoid.configuration.compactMaxWidth > 0
            Layout.alignment: Qt.AlignVCenter
            Layout.maximumWidth: Kirigami.Units.gridUnit * Plasmoid.configuration.compactMaxWidth
            text: root.focusName
            font.pixelSize: compact.valueSize * 0.92
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        PopChip {
            visible: Plasmoid.configuration.compactShowCpu && compact.proc !== null
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: "text"
            label: i18n("CPU")
            widestValue: "100%"
            value: compact.proc ? Math.round(compact.proc.cpu) + "%" : ""
            valueColor: Style.heat(compact.proc ? compact.proc.cpu : 0, 60, 85, Kirigami.Theme)
        }
        PopChip {
            visible: Plasmoid.configuration.compactShowGpu && compact.proc !== null
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: "text"
            label: i18n("GPU")
            widestValue: "100%"
            value: compact.proc ? Math.round(compact.proc.gpu) + "%" : ""
            valueColor: Style.heat(compact.proc ? compact.proc.gpu : 0, 90, 99, Kirigami.Theme)
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
