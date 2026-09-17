import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.plasma.components as PlasmaComponents
import "lib/Highlight.js" as Highlight

Item {
    id: tile

    property var iconSource
    property string label
    property string query
    property string subtitle
    property color subtitleColor: Kirigami.Theme.textColor
    property string trailing
    property color trailingColor: Kirigami.Theme.textColor
    property bool selected: false
    property int iconSize: Kirigami.Units.iconSizes.medium
    property bool monochrome: false
    property bool roundIcon: false
    property color ringColor: "transparent"
    signal clicked()
    signal rightClicked()
    signal hovered()

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: Kirigami.Units.cornerRadius * 2.5
        color: tile.selected ? launcher.selectedFill : mouse.containsMouse ? launcher.hoverFill : "transparent"
        border.width: tile.selected ? 1 : 0
        border.color: launcher.selectedLine
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Kirigami.Units.largeSpacing
        anchors.rightMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            visible: !tile.roundIcon
            Layout.preferredWidth: tile.iconSize
            Layout.preferredHeight: tile.iconSize
            source: tile.roundIcon ? "" : tile.iconSource
            fallback: "application-x-executable"
            color: Kirigami.Theme.textColor
            isMask: tile.monochrome
        }
        Item {
            visible: tile.roundIcon
            Layout.preferredWidth: tile.iconSize
            Layout.preferredHeight: tile.iconSize
            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: "transparent"
                border.width: 2
                border.color: tile.ringColor
            }
            Components.Avatar {
                anchors.fill: parent
                anchors.margins: 3
                source: tile.roundIcon ? tile.iconSource : ""
                name: tile.label
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            PlasmaComponents.Label {
                Layout.fillWidth: true
                text: tile.query !== "" ? Highlight.mark(tile.label, tile.query, Kirigami.Theme.textColor) : tile.label
                textFormat: tile.query !== "" ? Text.StyledText : Text.PlainText
                elide: Text.ElideRight
            }
            PlasmaComponents.Label {
                visible: tile.subtitle !== ""
                Layout.fillWidth: true
                text: tile.subtitle
                elide: Text.ElideRight
                font: Kirigami.Theme.smallFont
                color: tile.subtitleColor
                opacity: 0.75
            }
        }
        PlasmaComponents.Label {
            visible: tile.trailing !== ""
            text: tile.trailing
            color: tile.trailingColor
            font: Kirigami.Theme.smallFont
        }
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
