import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ListView {
    id: pinsView
    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true
    spacing: Kirigami.Units.smallSpacing
    boundsBehavior: Flickable.StopAtBounds
    interactive: contentHeight > height
    flickDeceleration: 4000
    maximumFlickVelocity: 2400
    model: launcherData.sidebarPins
    delegate: RailPin {}
    onMovingChanged: if (moving) launcher.hoveredPin = null

    Rectangle {
        parent: pinsView
        z: -1
        anchors.fill: parent
        radius: Kirigami.Units.cornerRadius * 2
        color: launcher.hoverFill
        border.width: 1
        border.color: launcher.hairline
        opacity: launcher.sidebarDrag !== null && launcher.sidebarDrag.from < 0 && launcher.sidebarDrag.over ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 120 } }
    }
    Rectangle {
        readonly property var drag: launcher.sidebarDrag
        visible: drag !== null && drag.over && !(drag.from >= 0 && (drag.index === drag.from || drag.index === drag.from + 1))
        x: pinsView.width * 0.15
        width: pinsView.width * 0.7
        height: 2
        radius: 1
        y: drag ? Math.max(0, drag.index * (launcher.railPinHeight + pinsView.spacing) - pinsView.spacing / 2 - 1) : 0
        color: launcher.ink
    }
}
