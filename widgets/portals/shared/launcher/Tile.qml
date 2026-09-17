import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import "lib"

Item {
    id: tile

    property var iconSource
    property string label
    property string subtitle
    property bool selected: false
    property bool badge: false
    property int iconSize: 48
    property bool monochrome: false
    property var game: null
    property bool reorderable: false
    property bool dropTarget: false
    property bool dropInto: false
    property var folderIcons: []
    property var sidebarEntry: null
    readonly property bool isFolder: folderIcons.length > 0
    readonly property bool dragging: mouse.dragging
    signal reorderMove(point position)
    signal reorderDrop(point position)
    signal reorderCancel()
    signal clicked()
    signal rightClicked()
    signal hovered()

    opacity: dragging ? 0.45 : 1

    Rectangle {
        anchors.fill: parent
        anchors.margins: 3
        radius: Kirigami.Units.cornerRadius * 2.5
        color: tile.selected || (tile.dropTarget && tile.dropInto) ? launcher.selectedFill : mouse.containsMouse ? launcher.hoverFill : "transparent"
        border.width: tile.selected || (tile.dropTarget && tile.dropInto) ? 1 : 0
        border.color: tile.dropTarget ? Qt.alpha(Kirigami.Theme.textColor, 0.8) : launcher.selectedLine
    }
    Rectangle {
        visible: tile.dropTarget && !tile.dropInto
        width: 3
        radius: 1.5
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: Kirigami.Units.largeSpacing
        anchors.bottomMargin: Kirigami.Units.largeSpacing
        color: Kirigami.Theme.textColor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.smallSpacing * 2
        spacing: Kirigami.Units.smallSpacing

        Item {
            Layout.alignment: Qt.AlignHCenter
            readonly property bool art: !tile.isFolder && tile.game !== null && tile.game.appid !== ""
            Layout.preferredWidth: art ? Math.round(tile.iconSize * 0.84) : tile.iconSize
            Layout.preferredHeight: art ? Math.round(tile.iconSize * 1.26) : tile.iconSize
            Layout.topMargin: art ? -Math.round(tile.iconSize * 0.13) : 0
            Layout.bottomMargin: art ? -Math.round(tile.iconSize * 0.13) : 0
            Loader {
                anchors.fill: parent
                active: parent.art
                sourceComponent: GameArt {
                    game: tile.game || ({})
                    wide: false
                    showLogo: false
                    radius: Kirigami.Units.cornerRadius
                    scale: mouse.pressed ? 0.92 : 1
                }
            }
            Rectangle {
                visible: tile.isFolder
                anchors.fill: parent
                radius: Kirigami.Units.cornerRadius * 2
                color: Qt.alpha(Kirigami.Theme.textColor, tile.dropTarget && tile.dropInto ? 0.2 : 0.1)
                border.width: 1
                border.color: Qt.alpha(Kirigami.Theme.textColor, 0.14)
                scale: mouse.pressed ? 0.92 : 1
                Grid {
                    anchors.centerIn: parent
                    columns: 2
                    spacing: Math.round(tile.iconSize * 0.06)
                    Repeater {
                        model: tile.folderIcons.slice(0, 4)
                        Kirigami.Icon {
                            required property var modelData
                            width: Math.round(tile.iconSize * 0.36)
                            height: width
                            source: modelData
                            fallback: "application-x-executable"
                        }
                    }
                }
            }
            Kirigami.Icon {
                visible: !tile.isFolder && (tile.game === null || !tile.game.appid)
                anchors.fill: parent
                source: tile.iconSource
                fallback: "application-x-executable"
                color: Kirigami.Theme.textColor
                isMask: tile.monochrome
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
                color: Kirigami.Theme.textColor
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
        property point pressPoint
        property bool dragging: false
        property bool suppressClick: false
        anchors.fill: parent
        hoverEnabled: true
        preventStealing: tile.reorderable || dragging
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onEntered: tile.hovered()
        onPressed: function(event) {
            pressPoint = Qt.point(event.x, event.y)
            suppressClick = false
        }
        onPositionChanged: function(event) {
            if (!(event.buttons & Qt.LeftButton) || (!tile.reorderable && !tile.sidebarEntry))
                return
            const dx = Math.abs(event.x - pressPoint.x)
            const dy = Math.abs(event.y - pressPoint.y)
            if (!dragging && Math.hypot(dx, dy) > Qt.styleHints.startDragDistance && (tile.reorderable || dx > dy))
                dragging = true
            if (!dragging)
                return
            if (tile.sidebarEntry)
                launcher.sidebarDragMove(mouse, event.x, event.y, tile.sidebarEntry, -1, tile.iconSource)
            if (tile.reorderable)
                tile.reorderMove(Qt.point(event.x, event.y))
        }
        onReleased: function(event) {
            if (!dragging)
                return
            dragging = false
            suppressClick = true
            if (tile.sidebarEntry && launcher.sidebarDragEnd())
                tile.reorderCancel()
            else if (tile.reorderable)
                tile.reorderDrop(Qt.point(event.x, event.y))
        }
        onCanceled: {
            if (dragging) {
                launcher.sidebarDragCancel()
                tile.reorderCancel()
            }
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
}
