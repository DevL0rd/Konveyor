import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

PlasmaCore.ToolTipArea {
    id: button

    required property TaskLook look
    property var windows: []
    property var window: null
    property var launcher: null
    property string groupTitle
    property string badge
    property bool showBadge: false
    property bool dragging: false
    property Item dragSpace
    property real across: look.thickness

    readonly property bool idle: windows.length === 0
    readonly property bool grouped: window === null && windows.length > 1
    readonly property bool focused: windows.some(each => each.active)
    readonly property bool attention: windows.some(each => each.attention)
    readonly property bool minimized: windows.length > 0 && windows.every(each => each.minimized)
    readonly property var iconSource: windows.length > 0 ? windows[0].icon : launcher ? launcher.icon : "application-x-executable"
    readonly property string appName: windows.length > 0 ? (windows[0].appName || windows[0].title) : launcher ? (launcher.name || "") : ""
    readonly property bool horizontalEdge: look.edge === PlasmaCore.Types.TopEdge || look.edge === PlasmaCore.Types.BottomEdge
        || (look.edge !== PlasmaCore.Types.LeftEdge && look.edge !== PlasmaCore.Types.RightEdge)

    signal activated()
    signal middleClicked()
    signal menuRequested()
    signal picked(var window)
    signal dragMoved(point position, point grab)
    signal dragFinished()

    function bounce() {
        launchBounce.restart()
    }

    implicitWidth: look.vertical ? across : look.button
    implicitHeight: look.vertical ? look.button : across
    mainText: appName
    subText: grouped ? "" : windows.length === 1 && windows[0].title !== appName ? windows[0].title : idle ? i18n("Pinned — click to open") : ""
    icon: grouped ? "" : iconSource
    mainItem: grouped ? groupList : null
    interactive: grouped
    location: Plasmoid.location
    active: look.tooltips && !dragging

    Item {
        visible: false

        GroupList {
            id: groupList
            title: button.appName
            windows: button.grouped ? button.windows : []
            onPicked: window => button.picked(window)
        }
    }

    Rectangle {
        id: surface
        anchors.centerIn: parent
        width: button.look.vertical ? button.look.iconSize + button.look.padding * 2 : button.width - 2
        height: button.look.vertical ? button.height - 2 : button.look.iconSize + button.look.padding * 2
        radius: Kirigami.Units.cornerRadius * 1.5
        readonly property bool filled: button.focused && button.look.highlightStyle === 0
        readonly property bool outlined: button.focused && button.look.highlightStyle === 1
        color: button.attention ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.highlightColor
        opacity: button.dragging ? 0.4 : button.attention ? attentionPulse.level : filled ? (mouse.containsMouse ? 0.32 : 0.24)
            : mouse.containsMouse ? 0.14 : 0
        border.width: filled ? 1 : 0
        border.color: Kirigami.Theme.highlightColor

        Behavior on opacity {
            enabled: !button.attention
            NumberAnimation { duration: button.look.shortDuration; easing.type: Easing.OutCubic }
        }
        Behavior on color {
            ColorAnimation { duration: button.look.longDuration }
        }
    }

    Rectangle {
        anchors.fill: surface
        radius: surface.radius
        color: "transparent"
        visible: surface.outlined
        border.width: 1.5
        border.color: Kirigami.Theme.highlightColor
    }

    QtObject {
        id: attentionPulse
        property real level: 0.3
    }

    SequentialAnimation {
        running: button.attention && button.look.pulse && button.look.animate
        loops: Animation.Infinite
        onRunningChanged: if (!running) attentionPulse.level = 0.3
        NumberAnimation { target: attentionPulse; property: "level"; from: 0.15; to: 0.5; duration: 700; easing.type: Easing.InOutSine }
        NumberAnimation { target: attentionPulse; property: "level"; from: 0.5; to: 0.15; duration: 700; easing.type: Easing.InOutSine }
    }

    Kirigami.Icon {
        id: appIcon
        anchors.centerIn: parent
        width: button.look.iconSize
        height: width
        source: button.iconSource
        active: mouse.containsMouse
        opacity: button.idle || button.minimized ? 0.72 : 1
        scale: mouse.pressed ? 0.9 : mouse.containsMouse && button.look.animate ? 1.06 : 1
        transform: Translate { id: lift }

        Behavior on scale {
            NumberAnimation { duration: button.look.shortDuration; easing.type: Easing.OutCubic }
        }
        Behavior on opacity {
            NumberAnimation { duration: button.look.shortDuration }
        }
    }

    SequentialAnimation {
        id: launchBounce
        loops: 2
        readonly property real reach: button.look.iconSize * 0.25 * (button.look.location === PlasmaCore.Types.TopEdge || button.look.location === PlasmaCore.Types.LeftEdge ? 1 : -1)
        NumberAnimation { target: lift; property: button.look.vertical ? "x" : "y"; to: launchBounce.reach; duration: button.look.animate ? 180 : 0; easing.type: Easing.OutQuad }
        NumberAnimation { target: lift; property: button.look.vertical ? "x" : "y"; to: 0; duration: button.look.animate ? 260 : 0; easing.type: Easing.OutBounce }
    }

    TaskIndicator {
        look: button.look
        count: button.windows.length
        active: button.focused
        attention: button.attention
        minimized: button.minimized
        length: button.look.iconSize
        anchors.horizontalCenter: button.horizontalEdge ? parent.horizontalCenter : undefined
        anchors.verticalCenter: button.horizontalEdge ? undefined : parent.verticalCenter
        anchors.bottom: button.horizontalEdge && button.look.edge !== PlasmaCore.Types.TopEdge ? parent.bottom : undefined
        anchors.top: button.look.edge === PlasmaCore.Types.TopEdge ? parent.top : undefined
        anchors.left: button.look.edge === PlasmaCore.Types.LeftEdge ? parent.left : undefined
        anchors.right: button.look.edge === PlasmaCore.Types.RightEdge ? parent.right : undefined
        anchors.margins: Math.max(1, Math.round((button.across - button.look.iconSize - button.look.padding * 2) / 4))
    }

    ShortcutBadge {
        label: button.badge
        shown: button.showBadge
        animate: button.look.animate
        anchors.horizontalCenter: surface.right
        anchors.verticalCenter: surface.top
        anchors.horizontalCenterOffset: -width / 4
        anchors.verticalCenterOffset: height / 4
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
                button.middleClicked()
            else if (mouseEvent.button === Qt.RightButton)
                button.menuRequested()
            else
                button.activated()
        }
    }
}
