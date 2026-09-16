import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

Item {
    id: tile

    property var iconSource
    property string label
    property string subtitle
    property bool selected: false
    property bool badge: false
    property int iconSize: 48
    signal clicked()
    signal rightClicked()
    signal hovered()

    Rectangle {
        anchors.fill: parent
        anchors.margins: 3
        radius: Kirigami.Units.cornerRadius * 2
        color: tile.selected ? Qt.alpha(Kirigami.Theme.highlightColor, 0.22)
             : mouse.containsMouse ? Qt.alpha(Kirigami.Theme.textColor, 0.07) : "transparent"
        border.width: tile.selected ? 1 : 0
        border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.6)
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.smallSpacing * 2
        spacing: Kirigami.Units.smallSpacing

        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: tile.iconSize
            Layout.preferredHeight: tile.iconSize
            Kirigami.Icon {
                anchors.fill: parent
                source: tile.iconSource
                fallback: "application-x-executable"
                scale: mouse.pressed ? 0.92 : 1
                Behavior on scale { NumberAnimation { duration: 90 } }
            }
            Rectangle {
                visible: tile.badge
                width: Math.round(tile.iconSize * 0.24)
                height: width
                radius: width / 2
                anchors.right: parent.right
                anchors.top: parent.top
                color: Kirigami.Theme.highlightColor
                border.width: 2
                border.color: Kirigami.Theme.backgroundColor
            }
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: tile.label
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.Wrap
            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.92
        }
        PlasmaComponents.Label {
            visible: tile.subtitle !== ""
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: tile.subtitle
            elide: Text.ElideRight
            font: Kirigami.Theme.smallFont
            opacity: 0.6
        }
        Item { Layout.fillHeight: true }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onEntered: tile.hovered()
        onClicked: function(event) {
            if (event.button === Qt.RightButton)
                tile.rightClicked()
            else
                tile.clicked()
        }
    }
}
