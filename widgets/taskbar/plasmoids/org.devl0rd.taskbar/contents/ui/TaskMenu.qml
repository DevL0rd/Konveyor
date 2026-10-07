import QtQuick
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid

PlasmaExtras.Menu {
    id: menu

    property var entries: []

    signal windowPicked(var window)
    signal newInstance(var item)
    signal pinToggled(var item)
    signal closeAll(var item)

    function show(item, button) {
        for (const entry of entries)
            entry.destroy()
        clearMenuItems()
        const created = []
        const add = (properties, handler) => {
            const entry = entryComponent.createObject(menu, properties)
            entry.clicked.connect(handler)
            addMenuItem(entry)
            created.push(entry)
        }
        for (const window of item.windows)
            add({ text: window.title, icon: window.icon, checkable: true, checked: window.active }, () => menu.windowPicked(window))
        if (item.windows.length > 0)
            add({ separator: true }, () => {})
        add({ text: i18n("Open New Window"), icon: "window-new" }, () => menu.newInstance(item))
        add({ text: item.pinned ? i18n("Unpin from Taskbar") : i18n("Pin to Taskbar"), icon: item.pinned ? "window-unpin" : "window-pin" },
            () => menu.pinToggled(item))
        if (item.windows.length > 0)
            add({ text: item.windows.length > 1 ? i18n("Close All") : i18n("Close"), icon: "window-close" }, () => menu.closeAll(item))
        entries = created
        visualParent = button
        openRelative()
    }

    placement: {
        switch (Plasmoid.location) {
        case PlasmaCore.Types.TopEdge:
            return PlasmaExtras.Menu.BottomPosedLeftAlignedPopup
        case PlasmaCore.Types.LeftEdge:
            return PlasmaExtras.Menu.RightPosedTopAlignedPopup
        case PlasmaCore.Types.RightEdge:
            return PlasmaExtras.Menu.LeftPosedTopAlignedPopup
        default:
            return PlasmaExtras.Menu.TopPosedLeftAlignedPopup
        }
    }

    property Component entryComponent: Component {
        PlasmaExtras.MenuItem {}
    }
}
