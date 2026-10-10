import QtQuick
import "TaskOrder.js" as TaskOrder

ListView {
    id: strip

    required property TaskLook look
    property var items: []
    property var itemBadges: ({})
    property bool showBadges: false
    property bool wheelCycles: false
    property var itemsByKey: ({})
    property string dragKey
    property int dragFrom: -1
    property int dropIndex: -1
    property real dragOffset: 0
    property real dragLength: 0
    property bool settling: false
    property bool holding: false

    signal itemActivated(var entry, var window, Item button)
    signal itemMiddleClicked(var entry, var window)
    signal menuRequested(var entry, var window, Item button)
    signal windowPicked(var window)
    signal stepped(int step)
    signal reordered(var keys, string movedKey)

    function sync() {
        if (dragKey || holding)
            return
        itemsByKey = TaskOrder.keyed(items, entry => entry.key)
        TaskOrder.syncKeys(keys, items.map(entry => entry.key))
    }

    function release() {
        holding = false
        sync()
    }

    function startOf(index) {
        const item = itemAtIndex(index)
        return item ? (look.vertical ? item.y : item.x) : 0
    }

    function extentOf(index) {
        const item = itemAtIndex(index)
        return item ? (look.vertical ? item.height : item.width) : 0
    }

    function dragTo(index, key, position, grab) {
        const along = look.vertical ? position.y + contentY : position.x + contentX
        if (!dragKey) {
            dragFrom = index
            dragKey = key
            dragLength = extentOf(index) + spacing
        }
        dragOffset = along - (look.vertical ? grab.y : grab.x) - startOf(dragFrom)
        const center = startOf(dragFrom) + dragOffset + extentOf(dragFrom) / 2
        let target = 0
        for (let i = 0; i < keys.count; ++i)
            if (i !== dragFrom && startOf(i) + extentOf(i) / 2 < center)
                ++target
        dropIndex = target
    }

    function shiftOf(index) {
        if (!dragKey || index === dragFrom)
            return 0
        if (dragFrom < dropIndex && index > dragFrom && index <= dropIndex)
            return -dragLength
        if (dragFrom > dropIndex && index >= dropIndex && index < dragFrom)
            return dragLength
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

    orientation: look.vertical ? ListView.Vertical : ListView.Horizontal
    interactive: !dragKey && (look.vertical ? contentHeight > height : contentWidth > width)
    currentIndex: -1
    highlightFollowsCurrentItem: false
    keyNavigationEnabled: false
    boundsBehavior: Flickable.StopAtBounds
    clip: true
    spacing: look.spacing
    cacheBuffer: 10000
    implicitWidth: look.vertical ? look.thickness : contentWidth
    implicitHeight: look.vertical ? contentHeight : look.thickness

    Timer {
        id: settled
        interval: 100
        onTriggered: strip.settling = false
    }

    WheelHandler {
        enabled: strip.wheelCycles
        property real pending: 0
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: event => {
            pending += event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x
            if (Math.abs(pending) >= 120) {
                strip.stepped(pending > 0 ? -1 : 1)
                pending = 0
            }
        }
    }

    model: ListModel {
        id: keys
    }

    delegate: TaskEntry {
        id: entryDelegate
        required property int index
        required property string key
        look: strip.look
        shift: dragging ? strip.dragOffset : strip.shiftOf(index)
        animateShift: !dragging && !strip.settling
        entry: strip.itemsByKey[key] || null
        dragSpace: strip
        dragging: strip.dragKey === key
        badge: TaskOrder.itemBadge(index, strip.itemBadges)
        showBadge: strip.showBadges
        onActivated: (window, button) => strip.itemActivated(entry, window, button)
        onMiddleClicked: window => strip.itemMiddleClicked(entry, window)
        onMenuRequested: (window, button) => strip.menuRequested(entry, window, button)
        onPicked: window => strip.windowPicked(window)
        onDragMoved: (position, grab) => strip.dragTo(index, key, position, grab)
        onDragFinished: strip.finishDrag()
    }

    add: Transition {
        enabled: strip.look.animate
        NumberAnimation { property: "scale"; from: 0.4; to: 1; duration: strip.look.longDuration; easing.type: Easing.OutBack }
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: strip.look.longDuration }
    }
    remove: Transition {
        enabled: strip.look.animate
        NumberAnimation { property: "scale"; to: 0.4; duration: strip.look.shortDuration }
        NumberAnimation { property: "opacity"; to: 0; duration: strip.look.shortDuration }
    }
    move: Transition {
        enabled: !strip.settling && strip.look.animate
        NumberAnimation { properties: "x,y"; duration: strip.look.longDuration; easing.type: Easing.OutCubic }
    }
    displaced: Transition {
        enabled: !strip.settling && strip.look.animate
        NumberAnimation { properties: "x,y"; duration: strip.look.longDuration; easing.type: Easing.OutCubic }
        NumberAnimation { properties: "scale,opacity"; to: 1; duration: strip.look.shortDuration }
    }
}
