import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

PlasmaCore.ToolTipArea {
    id: button

    required property var item
    property real size: 32
    property bool vertical: false
    property int edge: PlasmaCore.Types.BottomEdge
    property string badge
    property bool showBadge: false
    property bool dragging: false
    property Item dragSpace
    property real shift: 0
    property bool animateShift: true

    readonly property var windows: item ? item.windows : []
    readonly property bool idle: windows.length === 0
    readonly property bool focused: windows.some(window => window.active)
    readonly property bool attention: windows.some(window => window.attention)
    readonly property bool minimized: windows.length > 0 && windows.every(window => window.minimized)
    readonly property var launcher: item && item.launcher ? item.launcher : null
    readonly property var iconSource: windows.length > 0 ? windows[0].icon : launcher ? launcher.icon : "application-x-executable"
    readonly property string appName: windows.length > 0 ? (windows[0].appName || windows[0].title) : launcher ? (launcher.name || "") : ""

    signal activated()
    signal closeRequested()
    signal menuRequested()
    signal dragMoved(point position, point grab)
    signal dragFinished()

    function bounce() {
        launchBounce.restart()
    }

    implicitWidth: size
    implicitHeight: size
    mainText: appName
    subText: windows.length > 1 ? windows.map(window => "• " + window.title).join("\n") : windows.length === 1 && windows[0].title !== appName ? windows[0].title : idle ? i18n("Pinned — click to open") : ""
    icon: iconSource
    location: Plasmoid.location
    active: !dragging
    transform: Translate {
        x: button.vertical ? 0 : button.shift
        y: button.vertical ? button.shift : 0

        Behavior on x {
            enabled: button.animateShift
            NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
        }
        Behavior on y {
            enabled: button.animateShift
            NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
        }
    }

    Rectangle {
        id: surface
        anchors.fill: parent
        anchors.margins: Math.round(button.size * 0.06)
        radius: Kirigami.Units.cornerRadius * 1.5
        color: button.attention ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.highlightColor
        opacity: button.dragging ? 0.4 : button.attention ? attentionPulse.level : button.focused ? (mouse.containsMouse ? 0.32 : 0.24) : mouse.containsMouse ? 0.14 : 0
        border.width: button.focused ? 1 : 0
        border.color: Kirigami.Theme.highlightColor

        Behavior on opacity {
            enabled: !button.attention
            NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.OutCubic }
        }
        Behavior on color {
            ColorAnimation { duration: Kirigami.Units.longDuration }
        }
    }

    QtObject {
        id: attentionPulse
        property real level: 0.25
    }

    SequentialAnimation {
        running: button.attention
        loops: Animation.Infinite
        NumberAnimation { target: attentionPulse; property: "level"; from: 0.15; to: 0.5; duration: 700; easing.type: Easing.InOutSine }
        NumberAnimation { target: attentionPulse; property: "level"; from: 0.5; to: 0.15; duration: 700; easing.type: Easing.InOutSine }
    }

    Kirigami.Icon {
        id: appIcon
        anchors.centerIn: parent
        width: Math.round(button.size * 0.6)
        height: width
        source: button.iconSource
        active: mouse.containsMouse
        opacity: button.idle || button.minimized ? 0.72 : 1
        scale: mouse.pressed ? 0.88 : mouse.containsMouse ? 1.08 : 1
        transform: Translate { id: lift }

        Behavior on scale {
            NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.OutCubic }
        }
        Behavior on opacity {
            NumberAnimation { duration: Kirigami.Units.shortDuration }
        }
    }

    SequentialAnimation {
        id: launchBounce
        loops: 2
        NumberAnimation { target: lift; property: button.vertical ? "x" : "y"; to: button.edge === PlasmaCore.Types.TopEdge || button.edge === PlasmaCore.Types.LeftEdge ? button.size * 0.16 : -button.size * 0.16; duration: 180; easing.type: Easing.OutQuad }
        NumberAnimation { target: lift; property: button.vertical ? "x" : "y"; to: 0; duration: 260; easing.type: Easing.OutBounce }
    }

    TaskIndicator {
        count: button.windows.length
        active: button.focused
        attention: button.attention
        minimized: button.minimized
        vertical: button.vertical
        length: button.size
        anchors.horizontalCenter: button.vertical ? undefined : parent.horizontalCenter
        anchors.verticalCenter: button.vertical ? parent.verticalCenter : undefined
        anchors.bottom: button.edge === PlasmaCore.Types.BottomEdge ? parent.bottom : undefined
        anchors.top: button.edge === PlasmaCore.Types.TopEdge ? parent.top : undefined
        anchors.left: button.edge === PlasmaCore.Types.LeftEdge ? parent.left : undefined
        anchors.right: button.edge === PlasmaCore.Types.RightEdge ? parent.right : undefined
        anchors.margins: Math.max(1, Math.round(button.size * 0.04))
    }

    ShortcutBadge {
        label: button.badge
        shown: button.showBadge
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Math.round(button.size * 0.04)
    }

    MouseArea {
        id: mouse

        property point pressedAt
        property bool moved: false

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        cursorShape: button.dragging ? Qt.ClosedHandCursor : Qt.ArrowCursor

        onPressed: mouseEvent => {
            pressedAt = Qt.point(mouseEvent.x, mouseEvent.y)
            moved = false
        }
        onPositionChanged: mouseEvent => {
            if (!(mouseEvent.buttons & Qt.LeftButton) || !button.dragSpace)
                return
            if (!moved && Math.hypot(mouseEvent.x - pressedAt.x, mouseEvent.y - pressedAt.y) < Qt.styleHints.startDragDistance)
                return
            moved = true
            button.dragMoved(mapToItem(button.dragSpace, mouseEvent.x, mouseEvent.y), pressedAt)
        }
        onReleased: {
            if (moved)
                button.dragFinished()
        }
        onCanceled: {
            if (moved) {
                moved = false
                button.dragFinished()
            }
        }
        onClicked: mouseEvent => {
            if (moved)
                return
            if (mouseEvent.button === Qt.MiddleButton)
                button.closeRequested()
            else if (mouseEvent.button === Qt.RightButton)
                button.menuRequested()
            else
                button.activated()
        }
    }
}
