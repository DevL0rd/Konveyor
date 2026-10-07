import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import "TaskOrder.js" as TaskOrder

ListView {
    id: strip

    property var items: []
    property real size: 32
    property bool vertical: false
    property int edge: PlasmaCore.Types.BottomEdge
    property var columnBadges: ({})
    property bool showBadges: false
    property var itemsByKey: ({})
    property string dragKey
    property int dragFrom: -1
    property int dropIndex: -1
    property real dragOffset: 0
    property bool settling: false
    property bool holding: false

    signal itemActivated(var item, Item button)
    signal itemClosed(var item)
    signal menuRequested(var item, Item button)
    signal reordered(var keys, string movedKey)

    function badgeFor(item) {
        for (const column of (item ? item.columns : []))
            if (columnBadges[column.index])
                return columnBadges[column.index]
        return ""
    }

    function sync() {
        if (dragKey || holding)
            return
        itemsByKey = TaskOrder.keyed(items, item => item.key)
        TaskOrder.syncKeys(keys, items.map(item => item.key))
    }

    function release() {
        holding = false
        sync()
    }

    function dragTo(index, key, position, grab) {
        const along = vertical ? position.y + contentY : position.x + contentX
        if (!dragKey) {
            dragFrom = index
            dragKey = key
        }
        dragOffset = along - (vertical ? grab.y : grab.x) - index * size
        dropIndex = Math.max(0, Math.min(keys.count - 1, Math.floor(along / size)))
    }

    function shiftOf(index) {
        if (!dragKey || index === dragFrom)
            return 0
        if (dragFrom < dropIndex && index > dragFrom && index <= dropIndex)
            return -size
        if (dragFrom > dropIndex && index >= dropIndex && index < dragFrom)
            return size
        return 0
    }

    function finishDrag() {
        const moved = dragKey
        settling = true
        if (dropIndex !== dragFrom)
            keys.move(dragFrom, dropIndex, 1)
        dragKey = ""
        settled.restart()
        const order = []
        for (let i = 0; i < keys.count; ++i)
            order.push(keys.get(i).key)
        holding = true
        reordered(order, moved)
    }

    onItemsChanged: sync()

    orientation: vertical ? ListView.Vertical : ListView.Horizontal
    interactive: !dragKey && (vertical ? contentHeight > height : contentWidth > width)
    currentIndex: -1
    highlightFollowsCurrentItem: false
    keyNavigationEnabled: false
    boundsBehavior: Flickable.StopAtBounds
    clip: true
    spacing: 0
    implicitWidth: vertical ? size : contentWidth
    implicitHeight: vertical ? contentHeight : size

    Timer {
        id: settled
        interval: 100
        onTriggered: strip.settling = false
    }

    model: ListModel {
        id: keys
    }

    delegate: TaskButton {
        id: delegateButton
        required property int index
        required property string key
        shift: dragging ? strip.dragOffset : strip.shiftOf(index)
        animateShift: !dragging && !strip.settling
        item: strip.itemsByKey[key] || null
        size: strip.size
        vertical: strip.vertical
        edge: strip.edge
        dragSpace: strip
        dragging: strip.dragKey === key
        badge: strip.badgeFor(item)
        showBadge: strip.showBadges
        z: dragging ? 5 : 0
        onActivated: strip.itemActivated(item, delegateButton)
        onCloseRequested: strip.itemClosed(item)
        onMenuRequested: strip.menuRequested(item, delegateButton)
        onDragMoved: (position, grab) => strip.dragTo(index, key, position, grab)
        onDragFinished: strip.finishDrag()
    }

    add: Transition {
        NumberAnimation { property: "scale"; from: 0.4; to: 1; duration: Kirigami.Units.longDuration; easing.type: Easing.OutBack }
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Kirigami.Units.longDuration }
    }
    remove: Transition {
        NumberAnimation { property: "scale"; to: 0.4; duration: Kirigami.Units.shortDuration }
        NumberAnimation { property: "opacity"; to: 0; duration: Kirigami.Units.shortDuration }
    }
    move: Transition {
        enabled: !strip.settling
        NumberAnimation { properties: "x,y"; duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
    }
    displaced: Transition {
        enabled: !strip.settling
        NumberAnimation { properties: "x,y"; duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
        NumberAnimation { properties: "scale,opacity"; to: 1; duration: Kirigami.Units.shortDuration }
    }
}
