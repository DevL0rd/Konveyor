import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.plasma.components as PlasmaComponents
import "lib"
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
    property var game: null
    property bool emphasize: false
    property var sidebarEntry: null
    readonly property bool containsMouse: mouse.containsMouse
    default property alias extra: extraRow.data
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

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        property point pressPoint
        property bool dragging: false
        property bool suppressClick: false
        preventStealing: dragging
        onEntered: tile.hovered()
        onPressed: function(event) {
            pressPoint = Qt.point(event.x, event.y)
            suppressClick = false
        }
        onPositionChanged: function(event) {
            if (!(event.buttons & Qt.LeftButton) || !tile.sidebarEntry)
                return
            const dx = Math.abs(event.x - pressPoint.x)
            const dy = Math.abs(event.y - pressPoint.y)
            if (!dragging && Math.hypot(dx, dy) > Qt.styleHints.startDragDistance && dx > dy)
                dragging = true
            if (dragging)
                launcher.sidebarDragMove(mouse, event.x, event.y, tile.sidebarEntry, -1, tile.game ? "" : tile.iconSource)
        }
        onReleased: {
            if (!dragging)
                return
            dragging = false
            suppressClick = true
            launcher.sidebarDragEnd()
        }
        onCanceled: {
            if (dragging)
                launcher.sidebarDragCancel()
            dragging = false
        }
        onClicked: function(event) {
            if (suppressClick)
                return
            if (event.button === Qt.RightButton)
                tile.rightClicked()
            else
                tile.clicked()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Kirigami.Units.largeSpacing
        anchors.rightMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        Loader {
            active: tile.game !== null
            visible: active
            Layout.preferredHeight: Math.round(tile.height * 0.72)
            Layout.preferredWidth: Math.round(Layout.preferredHeight / 0.4667)
            sourceComponent: GameArt {
                game: tile.game || ({})
                wide: true
                showLogo: true
                radius: Kirigami.Units.cornerRadius * 1.5
            }
        }
        Kirigami.Icon {
            visible: !tile.roundIcon && tile.game === null
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
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * (tile.emphasize ? 1.5 : 1)
                font.weight: tile.emphasize ? Font.DemiBold : Font.Normal
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
        RowLayout {
            id: extraRow
            spacing: Kirigami.Units.smallSpacing
            z: 2
        }
    }

}
