import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import "lib"

MouseArea {
    id: compact

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property real thickness: vertical ? width : height
    readonly property real valueSize: Math.max(Kirigami.Theme.smallFont.pixelSize, Math.min(Kirigami.Theme.defaultFont.pixelSize * 1.05, thickness * 0.42))
    property bool wasExpanded: false

    function countText(n) {
        return n > 99 ? "99+" : n + ""
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
                source: Plasmoid.icon
            }
            Rectangle {
                width: Math.round(parent.width * 0.38)
                height: width
                radius: width / 2
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: -1
                color: root.stateColor
                border.width: 1.5
                border.color: Kirigami.Theme.backgroundColor
                visible: root.stateKey !== "live"
            }
        }

        PopChip {
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: "text"
            iconSource: "dialog-error-symbolic"
            widestValue: "99+"
            value: compact.countText(root.newErrors)
            valueColor: root.newErrors > 0 ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.disabledTextColor
            opacity: root.newErrors > 0 ? 1 : 0.45
        }
        PopChip {
            visible: Plasmoid.configuration.compactShowWarnings
            vertical: compact.vertical
            panelThickness: compact.thickness
            chipStyle: "text"
            iconSource: "dialog-warning-symbolic"
            widestValue: "99+"
            value: compact.countText(root.newWarnings)
            valueColor: root.newWarnings > 0 ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.disabledTextColor
            opacity: root.newWarnings > 0 ? 1 : 0.45
        }
    }
}
