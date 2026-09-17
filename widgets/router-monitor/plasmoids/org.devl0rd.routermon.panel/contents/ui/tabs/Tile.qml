import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../lib"

Rectangle {
    id: tile

    property alias label: stat.label
    property alias value: stat.value
    property alias unit: stat.unit
    property alias valueColor: stat.color
    property string caption
    property string icon
    signal clicked()

    Layout.fillWidth: true
    Layout.preferredWidth: 1
    implicitHeight: column.implicitHeight + Kirigami.Units.smallSpacing * 4
    radius: Kirigami.Units.cornerRadius * 2
    color: Qt.alpha(Kirigami.Theme.textColor, mouse.containsMouse ? 0.075 : 0.045)
    border.width: 1
    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.07)
    Behavior on color { ColorAnimation { duration: 140 } }

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: Kirigami.Units.smallSpacing * 2.5
        spacing: 2

        RowLayout {
            Layout.fillWidth: true
            PopStat {
                id: stat
                Layout.fillWidth: true
                scale: 1.35
            }
            Kirigami.Icon {
                visible: tile.icon !== ""
                source: tile.icon
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: Kirigami.Units.iconSizes.small
                Layout.alignment: Qt.AlignTop
                opacity: 0.45
            }
        }
        Text {
            visible: tile.caption !== ""
            Layout.fillWidth: true
            text: tile.caption
            color: Kirigami.Theme.textColor
            font: Kirigami.Theme.smallFont
            opacity: 0.6
            elide: Text.ElideRight
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: tile.clicked()
    }
}
