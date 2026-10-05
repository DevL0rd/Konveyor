import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

MouseArea {
    id: chip
    property string text
    property string count
    property string iconName
    property bool active: false
    implicitWidth: chipRow.implicitWidth + Kirigami.Units.smallSpacing * 3
    implicitHeight: chipRow.implicitHeight + Kirigami.Units.smallSpacing
    hoverEnabled: true
    cursorShape: Qt.PointingHandCursor
    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: chip.active ? Qt.alpha(Kirigami.Theme.highlightColor, 0.24)
             : Qt.alpha(Kirigami.Theme.textColor, chip.containsMouse ? 0.12 : 0.06)
        border.width: chip.active ? 1 : 0
        border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.5)
        Behavior on color { ColorAnimation { duration: 120 } }
    }
    RowLayout {
        id: chipRow
        anchors.centerIn: parent
        spacing: Kirigami.Units.smallSpacing
        Kirigami.Icon {
            visible: chip.iconName !== ""
            source: chip.iconName
            Layout.preferredWidth: Kirigami.Units.iconSizes.small * 0.8
            Layout.preferredHeight: Layout.preferredWidth
            opacity: 0.7
        }
        PlasmaComponents.Label {
            text: chip.text
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            font.weight: chip.active ? Font.DemiBold : Font.Normal
        }
        PlasmaComponents.Label {
            visible: chip.count !== ""
            text: chip.count
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            font.features: { "tnum": 1 }
            opacity: 0.55
        }
    }
}
