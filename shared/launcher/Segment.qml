import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

MouseArea {
    id: segment
    property string text
    property string iconName
    property bool current: false
    property bool iconOnly: false
    implicitWidth: (iconOnly ? 0 : segmentLabel.implicitWidth) + (iconName !== "" ? Kirigami.Units.iconSizes.small + (iconOnly ? 0 : Kirigami.Units.smallSpacing) : 0) + Kirigami.Units.largeSpacing * 2
    implicitHeight: Kirigami.Units.gridUnit * 1.9
    hoverEnabled: true
    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: segment.current ? launcher.selectedFill : segment.containsMouse ? launcher.hoverFill : "transparent"
        border.width: segment.current ? 1 : 0
        border.color: launcher.hairline
    }
    RowLayout {
        anchors.centerIn: parent
        spacing: Kirigami.Units.smallSpacing
        Kirigami.Icon {
            visible: segment.iconName !== ""
            Layout.preferredWidth: Kirigami.Units.iconSizes.small
            Layout.preferredHeight: Layout.preferredWidth
            source: segment.iconName
            color: launcher.ink
            isMask: true
            opacity: segment.current ? 1 : 0.65
        }
        PlasmaComponents.Label {
            id: segmentLabel
            visible: !segment.iconOnly
            text: segment.text
            font.weight: segment.current ? Font.DemiBold : Font.Normal
            opacity: segment.current ? 1 : 0.7
        }
    }
    property bool tipDismissed: false
    onPressed: tipDismissed = true
    onExited: tipDismissed = false
    QQC2.ToolTip.visible: iconOnly && containsMouse && !tipDismissed
    QQC2.ToolTip.text: text
}
