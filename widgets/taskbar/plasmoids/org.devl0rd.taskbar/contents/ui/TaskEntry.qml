import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: entryItem

    required property TaskLook look
    property var entry: null
    property string badge
    property bool showBadge: false
    property bool dragging: false
    property Item dragSpace
    property real shift: 0
    property bool animateShift: true

    readonly property var windows: entry ? entry.windows : []
    readonly property bool capsule: !!entry && entry.kind === "column" && windows.length > 1
    readonly property int inset: Math.max(1, Math.round(look.padding / 2))
    readonly property real across: look.thickness

    signal activated(var window, Item button)
    signal middleClicked(var window)
    signal menuRequested(var window, Item button)
    signal picked(var window)
    signal dragMoved(point position, point grab)
    signal dragFinished()

    function bounce() {
        if (single.visible)
            single.bounce()
    }

    implicitWidth: look.vertical ? across : (capsule ? buttons.implicitWidth + inset * 2 : single.implicitWidth)
    implicitHeight: look.vertical ? (capsule ? buttons.implicitHeight + inset * 2 : single.implicitHeight) : across
    width: implicitWidth
    height: implicitHeight
    z: dragging ? 5 : 0
    transform: Translate {
        x: entryItem.look.vertical ? 0 : entryItem.shift
        y: entryItem.look.vertical ? entryItem.shift : 0

        Behavior on x {
            enabled: entryItem.animateShift
            NumberAnimation { duration: entryItem.look.longDuration; easing.type: Easing.OutCubic }
        }
        Behavior on y {
            enabled: entryItem.animateShift
            NumberAnimation { duration: entryItem.look.longDuration; easing.type: Easing.OutCubic }
        }
    }

    Rectangle {
        id: shell
        visible: entryItem.capsule
        anchors.centerIn: parent
        width: entryItem.look.vertical ? entryItem.look.iconSize + entryItem.look.padding * 2 + entryItem.inset * 2 : parent.width
        height: entryItem.look.vertical ? parent.height : entryItem.look.iconSize + entryItem.look.padding * 2 + entryItem.inset * 2
        radius: Kirigami.Units.cornerRadius * 2
        color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, entryItem.dragging ? 0.16 : 0.08)
        border.width: 1
        border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.14)
    }

    Grid {
        id: tabs
        visible: entryItem.capsule && !!entryItem.entry && entryItem.entry.tabbed
        spacing: 2
        rows: entryItem.look.vertical ? entryItem.windows.length : 1
        columns: entryItem.look.vertical ? 1 : entryItem.windows.length
        anchors.horizontalCenter: entryItem.look.vertical ? undefined : shell.horizontalCenter
        anchors.verticalCenter: entryItem.look.vertical ? shell.verticalCenter : undefined
        anchors.top: entryItem.look.vertical ? undefined : shell.top
        anchors.left: entryItem.look.vertical ? shell.left : undefined
        anchors.topMargin: 1
        anchors.leftMargin: 1

        Repeater {
            model: tabs.visible ? entryItem.windows : []

            Rectangle {
                required property var modelData
                readonly property int span: Math.max(4, Math.round(entryItem.look.iconSize / 3))
                width: entryItem.look.vertical ? 2 : span
                height: entryItem.look.vertical ? span : 2
                radius: 1
                color: modelData.active ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
                opacity: modelData.active ? 1 : 0.4
            }
        }
    }

    Grid {
        id: buttons
        visible: entryItem.capsule
        anchors.centerIn: parent
        rows: entryItem.look.vertical ? entryItem.windows.length : 1
        columns: entryItem.look.vertical ? 1 : entryItem.windows.length
        spacing: 0

        Repeater {
            model: entryItem.capsule ? entryItem.windows : []

            TaskButton {
                id: member
                required property var modelData
                look: entryItem.look
                across: entryItem.across - entryItem.inset * 2
                windows: [modelData]
                window: modelData
                dragging: entryItem.dragging
                dragSpace: entryItem.dragSpace
                onActivated: entryItem.activated(modelData, member)
                onMiddleClicked: entryItem.middleClicked(modelData)
                onMenuRequested: entryItem.menuRequested(modelData, member)
                onDragMoved: (position, grab) => entryItem.dragMoved(position, member.mapToItem(entryItem, grab.x, grab.y))
                onDragFinished: entryItem.dragFinished()
            }
        }
    }

    ShortcutBadge {
        visible: entryItem.capsule
        label: entryItem.badge
        shown: entryItem.showBadge
        animate: entryItem.look.animate
        anchors.horizontalCenter: shell.right
        anchors.verticalCenter: shell.top
        anchors.horizontalCenterOffset: -width / 4
        anchors.verticalCenterOffset: height / 4
    }

    TaskButton {
        id: single
        visible: !entryItem.capsule
        look: entryItem.look
        windows: entryItem.windows
        window: entryItem.entry && (entryItem.entry.kind === "column" || entryItem.entry.kind === "window") ? entryItem.windows[0] || null : null
        launcher: entryItem.entry && entryItem.entry.launcher ? entryItem.entry.launcher : null
        badge: entryItem.badge
        showBadge: entryItem.showBadge && !entryItem.capsule
        dragging: entryItem.dragging
        dragSpace: entryItem.dragSpace
        onActivated: entryItem.activated(window, single)
        onMiddleClicked: entryItem.middleClicked(window)
        onMenuRequested: entryItem.menuRequested(window, single)
        onPicked: window => entryItem.picked(window)
        onDragMoved: (position, grab) => entryItem.dragMoved(position, grab)
        onDragFinished: entryItem.dragFinished()
    }
}
