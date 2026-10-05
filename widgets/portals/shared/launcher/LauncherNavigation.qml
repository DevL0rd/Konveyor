import QtQuick
import org.kde.kirigami as Kirigami

LauncherMenus {
    id: launcher

    function liveSections() {
        const view = currentView()
        if (!view || !view.sections)
            return []
        return view.sections.filter(section => section && section.visible && section.shownCount > 0)
    }
    function applySection(sections, index, scroll) {
        const view = currentView()
        const all = view && view.sections ? view.sections : []
        for (const section of all) {
            if (section)
                section.sectionActive = false
        }
        sectionIndex = index
        const current = sections[index]
        if (!current)
            return
        current.sectionActive = true
        if (scroll !== false && view.column && current.currentIndex >= 0 && !current.scrolling)
            ensureVisible(view, current)
    }
    function ensureVisible(view, section) {
        const item = section.itemAtIndex(section.currentIndex)
        const flick = view.contentItem
        if (!item || !flick)
            return
        const top = item.mapToItem(view.column, 0, 0).y
        const headerRoom = section.currentIndex < section.columns ? Kirigami.Units.gridUnit * 2.4 : Kirigami.Units.largeSpacing
        const wantTop = top - headerRoom
        const wantBottom = top + item.height + Kirigami.Units.largeSpacing
        let target = flick.contentY
        if (wantTop < flick.contentY)
            target = wantTop
        else if (wantBottom > flick.contentY + flick.height)
            target = wantBottom - flick.height
        target = Math.max(0, Math.min(target, view.column.height - flick.height))
        if (Math.abs(target - flick.contentY) < 1)
            return
        scrollAnimation.target = flick
        scrollAnimation.to = target
        scrollAnimation.restart()
    }
    NumberAnimation {
        id: scrollAnimation
        property: "contentY"
        duration: Kirigami.Units.longDuration
        easing.type: Easing.OutCubic
    }
    function resetSelection() {
        railIndex = -1
        const sections = liveSections()
        if (sections.length === 0)
            return
        for (const section of sections)
            section.currentIndex = -1
        sections[0].reset()
        applySection(sections, 0)
    }
    function select(section, index) {
        if (railIndex >= 0)
            leaveRail()
        if (section.currentIndex === index && section.sectionActive)
            return
        const sections = liveSections()
        const at = sections.indexOf(section)
        if (at < 0)
            return
        if (sections[sectionIndex] && sectionIndex !== at)
            sections[sectionIndex].currentIndex = -1
        section.currentIndex = index
        applySection(sections, at, false)
    }
    function ensureSelection() {
        const current = currentSection()
        if (!current || current.currentIndex < 0 || current.currentIndex >= current.shownCount)
            resetSelection()
        else if (railIndex < 0 && !current.sectionActive)
            applySection(liveSections(), sectionIndex, false)
        activatePending()
    }
    function activatePending() {
        const section = currentSection()
        if (!activationPending || launcherData.runner.querying || !section || !section.currentItem)
            return
        activationPending = false
        section.activate()
    }
    function currentSection() {
        const sections = liveSections()
        if (sectionIndex >= sections.length)
            return null
        return sections[sectionIndex]
    }
    function enterRail() {
        const sections = liveSections()
        for (const section of sections)
            section.sectionActive = false
        railIndex = 0
        showPin(0)
    }
    function leaveRail() {
        railIndex = -1
        const sections = liveSections()
        if (sections[sectionIndex])
            applySection(sections, sectionIndex, false)
    }
    function showPin(index) {
        const step = railPinHeight + pinsView.spacing
        const top = index * step
        let target = pinsView.contentY
        if (top < pinsView.contentY)
            target = top
        else if (top + railPinHeight > pinsView.contentY + pinsView.height)
            target = top + railPinHeight - pinsView.height
        target = Math.max(0, Math.min(target, pinsView.contentHeight - pinsView.height))
        if (Math.abs(target - pinsView.contentY) < 1)
            return
        scrollAnimation.target = pinsView
        scrollAnimation.to = target
        scrollAnimation.restart()
    }
    function navigate(dx, dy) {
        if (railIndex >= 0) {
            const count = launcherData.sidebarPins.length
            if (dx > 0 || count === 0) {
                leaveRail()
            } else if (dy !== 0) {
                railIndex = Math.max(0, Math.min(count - 1, railIndex + dy))
                showPin(railIndex)
            }
            return
        }
        const sections = liveSections()
        if (sections.length === 0) {
            if (dx < 0 && launcherData.sidebarPins.length > 0)
                enterRail()
            return
        }
        if (sectionIndex >= sections.length) {
            resetSelection()
            return
        }
        const current = sections[sectionIndex]
        if (current.currentIndex < 0) {
            current.reset()
            applySection(sections, sectionIndex)
            return
        }
        const pins = launcherData.sidebarPins.length > 0
        if (dx < 0 && pins && current.columns > 0 && current.currentIndex % current.columns === 0) {
            enterRail()
            return
        }
        if (current.move(dx, dy)) {
            applySection(sections, sectionIndex)
            return
        }
        if (dx < 0 && pins) {
            enterRail()
            return
        }
        if (dy > 0 && sectionIndex + 1 < sections.length) {
            current.currentIndex = -1
            sections[sectionIndex + 1].enterFrom(false)
            applySection(sections, sectionIndex + 1)
        } else if (dy < 0 && sectionIndex > 0) {
            current.currentIndex = -1
            sections[sectionIndex - 1].enterFrom(true)
            applySection(sections, sectionIndex - 1)
        }
    }
    function stepSection(forward) {
        railIndex = -1
        const sections = liveSections()
        if (sections.length <= 1) {
            const view = currentView()
            if (view && view.cycle)
                view.cycle(forward)
            return
        }
        const next = (sectionIndex + (forward ? 1 : -1) + sections.length) % sections.length
        if (sections[sectionIndex])
            sections[sectionIndex].currentIndex = -1
        sections[next].reset()
        applySection(sections, next)
    }
    property var railPins: []
    Connections {
        target: launcherData
        function onSidebarPinsChanged() { launcher.followRailPin() }
    }
    function followRailPin() {
        const pins = launcherData.sidebarPins
        const key = railIndex >= 0 ? launcherData.sidebarKey(railPins[railIndex]) : ""
        railPins = pins
        if (railIndex < 0)
            return
        const at = pins.findIndex(pin => launcherData.sidebarKey(pin) === key)
        railIndex = at >= 0 ? at : Math.min(railIndex, pins.length - 1)
    }
    function currentPin() {
        return railIndex >= 0 ? launcherData.sidebarPins[railIndex] || null : null
    }
    function activateCurrent() {
        const pin = currentPin()
        if (pin) {
            if (pin.missing)
                openMenu(sidebarEntries(pin, railIndex), pinsView.itemAtIndex(railIndex))
            else
                openPin(pin)
            return
        }
        const section = currentSection()
        if (section && (section.currentItem || !searching))
            section.activate()
        else
            activationPending = searching
    }
    function menuForCurrent() {
        const pin = currentPin()
        if (pin) {
            openMenu(sidebarEntries(pin, railIndex), pinsView.itemAtIndex(railIndex))
            return
        }
        const section = currentSection()
        if (section)
            section.openMenu()
    }
    function pinCurrent() {
        const section = currentSection()
        if (!section || section.currentIndex < 0)
            return
        const item = section.itemAtIndex(section.currentIndex)
        if (item && item.favoriteId)
            togglePin(item.favoriteId)
    }

    function sidebarPinCurrent() {
        const pin = currentPin()
        if (pin) {
            launcherData.removeSidebar(pin)
            railIndex = Math.min(railIndex, launcherData.sidebarPins.length - 1)
            if (railIndex < 0)
                leaveRail()
            return
        }
        const section = currentSection()
        if (!section || section.currentIndex < 0)
            return
        const item = section.itemAtIndex(section.currentIndex)
        if (item && item.sidebarEntry)
            launcherData.toggleSidebar(item.sidebarEntry)
    }
    function pinHovered(item, on) {
        if (on)
            hoveredPin = item
        else if (hoveredPin === item)
            hoveredPin = null
    }
    function sidebarDragMove(item, x, y, entry, from, icon) {
        if (!entry)
            return false
        const point = item.mapToItem(content, x, y)
        const inRail = item.mapToItem(rail, x, y)
        const inView = item.mapToItem(pinsView, x, y)
        const over = inRail.x >= -Kirigami.Units.largeSpacing && inRail.x <= rail.width + Kirigami.Units.largeSpacing && inRail.y >= 0 && inRail.y <= rail.height
        const count = launcherData.sidebarPins.length
        const step = railPinHeight + pinsView.spacing
        const slot = inView.y < 0 ? 0 : inView.y > pinsView.height ? count : Math.floor((inView.y + pinsView.contentY + step / 2) / step)
        sidebarDrag = {
            entry: entry,
            from: from,
            icon: icon === undefined ? entry.icon : icon,
            index: Math.max(0, Math.min(count, slot)),
            over: over,
            removing: from >= 0 && !over,
            x: point.x,
            y: point.y,
            edge: over ? (inView.y < railPinHeight * 0.6 ? -1 : inView.y > pinsView.height - railPinHeight * 0.6 ? 1 : 0) : 0,
            item: item,
            itemX: x,
            itemY: y
        }
        hoveredPin = null
        return over
    }
    function sidebarDragEnd() {
        const drag = sidebarDrag
        sidebarDrag = null
        if (!drag)
            return false
        if (drag.from >= 0) {
            if (drag.removing)
                launcherData.removeSidebar(drag.entry)
            else
                launcherData.moveSidebar(drag.from, drag.index > drag.from ? drag.index - 1 : drag.index)
            return true
        }
        if (!drag.over)
            return false
        const existing = launcherData.sidebarIndex(drag.entry)
        launcherData.addSidebar(drag.entry, existing >= 0 && existing < drag.index ? drag.index - 1 : drag.index)
        return true
    }
    function sidebarDragCancel() {
        sidebarDrag = null
    }
    Timer {
        interval: 16
        repeat: true
        running: launcher.sidebarDrag !== null && launcher.sidebarDrag.edge !== 0
        onTriggered: {
            const drag = launcher.sidebarDrag
            const limit = Math.max(0, pinsView.contentHeight - pinsView.height)
            const next = Math.max(0, Math.min(limit, pinsView.contentY + drag.edge * Kirigami.Units.gridUnit * 0.35))
            if (next === pinsView.contentY)
                return
            pinsView.contentY = next
            launcher.sidebarDragMove(drag.item, drag.itemX, drag.itemY, drag.entry, drag.from, drag.icon)
        }
    }
}
