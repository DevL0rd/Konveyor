import QtQuick
import org.kde.kirigami as Kirigami
import "lib"

MouseArea {
    id: pin

    required property int index
    required property var modelData
    readonly property var view: ListView.view
    readonly property var game: {
        launcherData.gameByDesktop
        return launcherData.sidebarGame(modelData)
    }
    readonly property bool art: game !== null && !!game.appid
    readonly property bool missing: modelData.missing === true
    readonly property bool selected: launcher.railIndex === index
    readonly property bool lifted: launcher.sidebarDrag !== null && launcher.sidebarDrag.from === index
    property point pressPoint
    property bool dragging: false
    property bool suppressClick: false
    property bool held: false

    width: view ? view.width : 0
    height: launcher.railPinHeight
    hoverEnabled: true
    preventStealing: !launcher.touchMode || dragging
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    opacity: lifted ? 0.3 : 1

    onContainsMouseChanged: launcher.pinHovered(pin, containsMouse)
    onPressed: function(event) {
        pressPoint = Qt.point(event.x, event.y)
        suppressClick = false
        held = false
    }
    onPressAndHold: function(event) {
        if (!launcher.touchMode || dragging) {
            event.accepted = false
            return
        }
        held = true
        launcher.railIndex = -1
        launcher.openMenu(launcher.sidebarEntries(modelData, index), pin)
    }
    onPositionChanged: function(event) {
        if (held || !(event.buttons & Qt.LeftButton))
            return
        const dx = Math.abs(event.x - pressPoint.x)
        const dy = Math.abs(event.y - pressPoint.y)
        if (!dragging && Math.hypot(dx, dy) > Qt.styleHints.startDragDistance && (!launcher.touchMode || !view.interactive || dx > dy))
            dragging = true
        if (dragging)
            launcher.sidebarDragMove(pin, event.x, event.y, modelData, index, modelData.icon)
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
        launcher.railIndex = -1
        if (event.button === Qt.RightButton || missing)
            launcher.openMenu(launcher.sidebarEntries(modelData, index), pin)
        else
            launcher.openPin(modelData)
    }

    Rectangle {
        anchors.fill: parent
        radius: Kirigami.Units.cornerRadius * 2
        color: pin.selected ? launcher.selectedFill : pin.containsMouse ? launcher.hoverFill : "transparent"
        border.width: pin.selected ? 1 : 0
        border.color: launcher.hairline
    }

    Item {
        anchors.centerIn: parent
        width: launcher.railPinIcon
        height: width
        opacity: pin.missing ? 0.38 : 1
        scale: pin.pressed && !pin.dragging ? 0.9 : 1
        Behavior on scale { NumberAnimation { duration: 90 } }

        Loader {
            anchors.fill: parent
            active: pin.art
            sourceComponent: GameArt {
                game: pin.game
                wide: false
                showLogo: false
                radius: Kirigami.Units.cornerRadius
            }
        }
        Kirigami.Icon {
            visible: !pin.art
            anchors.fill: parent
            source: pin.modelData.icon || (pin.modelData.kind === "path" ? "folder" : "application-x-executable")
            fallback: pin.modelData.generic || (pin.modelData.kind === "path" ? "text-x-generic" : "application-x-executable")
        }
    }
    Kirigami.Icon {
        visible: pin.missing
        width: Math.round(launcher.railPinIcon * 0.5)
        height: width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: Math.round(launcher.railPinIcon * 0.4)
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: Math.round(launcher.railPinIcon * 0.4)
        source: "emblem-unavailable"
    }
}
