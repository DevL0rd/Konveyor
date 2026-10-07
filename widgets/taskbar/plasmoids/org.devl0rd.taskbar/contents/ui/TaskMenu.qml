import QtQuick
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid

PlasmaExtras.Menu {
    id: menu

    property var created: []
    property var entry: null
    property var window: null

    signal actionTriggered(var action, var entry, var window)

    function clear() {
        for (const object of created)
            object.destroy()
        created = []
        clearMenuItems()
    }

    function fill(target, entries) {
        for (const each of entries) {
            const item = itemComponent.createObject(target, {
                text: each.hint ? (each.text || "") + "\t" + each.hint : each.text || "",
                icon: each.checkable ? "" : each.icon || "",
                checkable: !!each.checkable,
                checked: !!each.checked,
                separator: !!each.separator,
                section: !!each.section,
                enabled: each.enabled !== false
            })
            created.push(item)
            if (each.children) {
                const submenu = submenuComponent.createObject(menu)
                created.push(submenu)
                submenu.visualParent = item.action
                fill(submenu, each.children)
            } else if (each.action) {
                const action = each.action
                item.clicked.connect(() => menu.actionTriggered(action, menu.entry, menu.window))
            }
            target.addMenuItem(item)
        }
    }

    function show(entries, entry, window, button) {
        clear()
        menu.entry = entry
        menu.window = window
        fill(menu, entries)
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

    property Component itemComponent: Component {
        PlasmaExtras.MenuItem {}
    }

    property Component submenuComponent: Component {
        PlasmaExtras.Menu {}
    }
}
