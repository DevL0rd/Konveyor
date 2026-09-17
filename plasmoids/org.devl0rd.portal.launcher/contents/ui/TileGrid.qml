import QtQuick
import org.kde.kirigami as Kirigami

GridView {
    id: grid

    property bool sectionActive: false
    property int limit: -1
    property bool scrolling: false
    property int iconSize: 48
    property bool wideCards: false
    property bool showTitles: true
    property real cardSpacing: Kirigami.Units.largeSpacing * 0.75
    readonly property int shownCount: limit >= 0 ? Math.min(limit, count) : count
    readonly property int columns: Math.max(1, Math.floor(width / cellWidth))
    readonly property bool keyboardSelection: sectionActive && currentIndex >= 0

    interactive: scrolling
    implicitHeight: shownCount > 0 ? Math.ceil(shownCount / columns) * cellHeight : 0
    currentIndex: -1
    keyNavigationEnabled: false
    highlightFollowsCurrentItem: false
    boundsBehavior: Flickable.StopAtBounds
    clip: scrolling || limit >= 0
    onCountChanged: if (visible) Qt.callLater(launcher.ensureSelection)
    reuseItems: true
    cacheBuffer: scrolling ? cellHeight * 2 : 0

    function reset() {
        currentIndex = shownCount > 0 ? 0 : -1
    }
    function move(dx, dy) {
        if (shownCount === 0)
            return false
        if (currentIndex < 0) {
            currentIndex = 0
            return true
        }
        if (dx !== 0) {
            const next = currentIndex + dx
            if (next < 0 || next >= shownCount)
                return false
            currentIndex = next
        } else {
            const next = currentIndex + dy * columns
            if (next < 0)
                return false
            if (next >= shownCount) {
                if (Math.floor(currentIndex / columns) >= Math.floor((shownCount - 1) / columns))
                    return false
                currentIndex = shownCount - 1
            } else {
                currentIndex = next
            }
        }
        if (scrolling)
            positionViewAtIndex(currentIndex, GridView.Contain)
        return true
    }
    function enterFrom(fromBelow) {
        if (shownCount === 0)
            return false
        currentIndex = fromBelow ? Math.max(0, shownCount - 1 - ((shownCount - 1) % columns)) : 0
        if (scrolling)
            positionViewAtIndex(currentIndex, GridView.Contain)
        return true
    }
    function activate() {
        const item = currentIndex >= 0 && currentIndex < shownCount ? itemAtIndex(currentIndex) : null
        if (item)
            item.activate()
    }
    function openMenu() {
        const item = currentIndex >= 0 && currentIndex < shownCount ? itemAtIndex(currentIndex) : null
        if (item)
            item.openMenu()
    }
}
