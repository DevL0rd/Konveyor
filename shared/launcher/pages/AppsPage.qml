import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQml.Models
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import ".."

ColumnLayout {
    id: page

    readonly property var sections: [grid]
    readonly property int tileSize: Plasmoid.configuration.tileSize
    readonly property string sort: Plasmoid.configuration.appsSort
    readonly property bool listView: Plasmoid.configuration.appsView === "list"
    property int categoryRow: 0
    readonly property var categoryModel: launcherData.rootModel.count > categoryRow ? launcherData.rootModel.modelForRow(categoryRow) : null
    property var letters: []

    readonly property var sortDefs: [
        { key: "name", label: i18n("Name") },
        { key: "popular", label: i18n("Most used") },
        { key: "recent", label: i18n("Recently used") },
        { key: "installed", label: i18n("Recently installed") }
    ]

    spacing: Kirigami.Units.largeSpacing

    function cycle(forward) {
        const total = launcherData.rootModel.count
        let row = categoryRow
        for (let step = 0; step < total; ++step) {
            row = (row + (forward ? 1 : -1) + total) % total
            if (launcherData.rootModel.labelForRow(row) !== "") {
                categoryRow = row
                break
            }
        }
        currentView().positionViewAtBeginning()
        Qt.callLater(launcher.resetSelection)
    }
    function currentView() {
        return grid
    }
    function jump(row) {
        currentView().positionViewAtIndex(row, GridView.Beginning)
        launcher.select(currentView(), row)
    }
    function zoom(steps) {
        Plasmoid.configuration.tileSize = Math.max(32, Math.min(112, Plasmoid.configuration.tileSize + steps * 8))
    }

    function rankOf(entry) {
        const id = launcherData.desktopKey(entry.model.favoriteId || "")
        if (page.sort === "recent") {
            const rank = launcherData.recentRank[id]
            return rank === undefined ? 100000 : rank
        }
        if (page.sort === "popular") {
            const rank = launcherData.popularRank[id]
            return rank === undefined ? 100000 : rank
        }
        if (page.sort === "installed")
            return entry.model.isNewlyInstalled === true ? 0 : 1
        return 0
    }
    readonly property var activeItems: page.listView ? listItems : gridItems
    readonly property var activeGroup: page.listView ? listShown : gridShown

    property int hiddenShown: 0
    function arrange() {
        const items = activeItems.items
        const anyHidden = Object.keys(launcherData.hiddenSet).length > 0
        for (let i = 0; (anyHidden || page.hiddenShown > 0) && i < items.count; ++i) {
            const entry = items.get(i)
            const hidden = launcherData.isHidden(entry.model.favoriteId || "")
            if (hidden && entry.inShown)
                items.removeGroups(i, 1, "shown")
            else if (!hidden && !entry.inShown)
                items.addGroups(i, 1, "shown")
        }
        const group = activeGroup
        page.hiddenShown = items.count - group.count
        if (page.sort !== "name") {
            const order = []
            for (let i = 0; i < group.count; ++i)
                order.push({ rank: page.rankOf(group.get(i)), label: String(group.get(i).model.display || "").toLowerCase(), row: group.get(i).model.index })
            const sorted = order.slice().sort((a, b) => (a.rank - b.rank) || a.label.localeCompare(b.label))
            for (let target = 0; target < sorted.length; ++target) {
                if (group.get(target).model.index === sorted[target].row)
                    continue
                for (let i = target + 1; i < group.count; ++i) {
                    if (group.get(i).model.index === sorted[target].row) {
                        group.move(i, target, 1)
                        break
                    }
                }
            }
        }
        const rows = {}
        if (page.sort === "name") {
            const direct = page.hiddenShown === 0 && page.categoryModel
            for (let i = 0; i < group.count; ++i) {
                const label = String(direct ? page.categoryModel.labelForRow(i) : group.get(i).model.display || "")
                const first = label.charAt(0).toUpperCase()
                const key = /[A-Z]/.test(first) ? first : "#"
                if (rows[key] === undefined)
                    rows[key] = i
            }
        }
        page.letters = group.count > 0 && page.sort === "name"
            ? ["#"].concat("ABCDEFGHIJKLMNOPQRSTUVWXYZ".split("")).map(key => ({ key: key, row: rows[key] === undefined ? -1 : rows[key] }))
            : []
    }
    Timer {
        id: arrangeTimer
        interval: 0
        onTriggered: page.arrange()
    }
    onSortChanged: arrangeTimer.restart()
    onListViewChanged: arrangeTimer.restart()
    Connections {
        target: launcherData
        function onHiddenListChanged() { arrangeTimer.restart() }
    }
    Connections {
        target: launcherData
        function onRecentRankChanged() { if (page.sort === "recent") arrangeTimer.restart() }
        function onPopularRankChanged() { if (page.sort === "popular") arrangeTimer.restart() }
    }

    DelegateModel {
        id: gridItems
        model: page.listView ? null : page.categoryModel
        groups: DelegateModelGroup {
            id: gridShown
            name: "shown"
            includeByDefault: true
        }
        filterOnGroup: "shown"
        items.onChanged: arrangeTimer.restart()
        delegate: Tile {
            id: appTile
            required property var model
            required property int index
            readonly property var view: GridView.view
            readonly property string favoriteId: model.favoriteId || ""
            width: view ? view.cellWidth : 0
            height: view ? view.cellHeight : 0
            iconSize: page.tileSize
            iconSource: model.decoration
            label: model.display || ""
            badge: model.isNewlyInstalled === true
            game: launcherData.gameForApp(favoriteId)
            selected: GridView.isCurrentItem && !!view && view.sectionActive
            function activate() {
                launcher.trigger(page.categoryModel, model.index, favoriteId)
            }
            function openMenu() {
                launcher.openMenu(launcher.kickerEntries(page.categoryModel, model.index, model.hasActionList ? model.actionList : [], favoriteId), appTile)
            }
            onHovered: launcher.select(view, index)
            onClicked: activate()
            onRightClicked: {
                launcher.select(view, index)
                openMenu()
            }
        }
    }

    DelegateModel {
        id: listItems
        model: page.listView ? page.categoryModel : null
        groups: DelegateModelGroup {
            id: listShown
            name: "shown"
            includeByDefault: true
        }
        filterOnGroup: "shown"
        items.onChanged: arrangeTimer.restart()
        delegate: RowTile {
            id: appRow
            required property var model
            required property int index
            readonly property var view: GridView.view
            readonly property string favoriteId: model.favoriteId || ""
            readonly property var gameEntry: launcherData.gameForApp(favoriteId)
            width: view ? view.cellWidth : 0
            height: view ? view.cellHeight : 0
            iconSource: model.decoration
            iconSize: Math.max(Kirigami.Units.iconSizes.medium, Math.round(page.tileSize * 0.7))
            game: gameEntry && gameEntry.appid ? gameEntry : null
            label: model.display || ""
            subtitle: model.description || ""
            trailing: model.isNewlyInstalled === true ? i18n("New") : ""
            selected: GridView.isCurrentItem && !!view && view.sectionActive
            function activate() {
                launcher.trigger(page.categoryModel, model.index, favoriteId)
            }
            function openMenu() {
                launcher.openMenu(launcher.kickerEntries(page.categoryModel, model.index, model.hasActionList ? model.actionList : [], favoriteId), appRow)
            }
            onHovered: launcher.select(view, index)
            onClicked: activate()
            onRightClicked: {
                launcher.select(view, index)
                openMenu()
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing

        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: chips.implicitHeight
            contentWidth: chips.implicitWidth
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            SegmentGroup {
                id: chips
                Repeater {
                    model: launcherData.rootModel
                    delegate: Segment {
                        id: chip
                        required property int index
                        required property var model
                        visible: (model.display || "") !== ""
                        text: model.display || ""
                        current: page.categoryRow === index
                        onClicked: {
                            page.categoryRow = index
                            page.currentView().positionViewAtBeginning()
                            Qt.callLater(launcher.resetSelection)
                        }
                    }
                }
            }
        }

        PlasmaComponents.Label {
            text: i18np("%1 app", "%1 apps", page.activeGroup.count)
            opacity: 0.5
        }

        Segment {
            id: sortButton
            text: i18n("Sort: %1", (page.sortDefs.find(entry => entry.key === page.sort) || page.sortDefs[0]).label)
            iconName: "view-sort-symbolic"
            onClicked: sortMenu.popup(sortButton, 0, sortButton.height)
            QQC2.Menu {
                id: sortMenu
                popupType: QQC2.Popup.Window
                onClosed: launcher.focusSearch()
                Instantiator {
                    model: page.sortDefs
                    delegate: PlasmaComponents.MenuItem {
                        required property var modelData
                        text: modelData.label
                        checkable: true
                        checked: page.sort === modelData.key
                        onTriggered: {
                            Plasmoid.configuration.appsSort = modelData.key
                            Qt.callLater(launcher.resetSelection)
                        }
                    }
                    onObjectAdded: (index, object) => sortMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => sortMenu.removeItem(object)
                }
            }
        }

        SegmentGroup {
            Segment {
                iconOnly: true
                iconName: "zoom-out-symbolic"
                text: i18n("Smaller")
                onClicked: page.zoom(-1)
            }
            Segment {
                iconOnly: true
                iconName: "zoom-in-symbolic"
                text: i18n("Larger")
                onClicked: page.zoom(1)
            }
        }

        SegmentGroup {
            Segment {
                iconOnly: true
                iconName: "view-grid-symbolic"
                text: i18n("Grid")
                current: !page.listView
                onClicked: {
                    Plasmoid.configuration.appsView = "grid"
                    Qt.callLater(launcher.resetSelection)
                }
            }
            Segment {
                iconOnly: true
                iconName: "view-list-details-symbolic"
                text: i18n("List")
                current: page.listView
                onClicked: {
                    Plasmoid.configuration.appsView = "list"
                    Qt.callLater(launcher.resetSelection)
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Kirigami.Units.largeSpacing

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            WheelHandler {
                acceptedModifiers: Qt.ControlModifier
                onWheel: function(event) {
                    page.zoom(event.angleDelta.y > 0 ? 1 : -1)
                }
            }

            TileGrid {
                id: grid
                anchors.fill: parent
                scrolling: true
                cellWidth: page.listView ? Math.floor(width / Math.max(1, Math.floor(width / (Kirigami.Units.gridUnit * 22))))
                                         : Math.floor(width / Math.max(1, Math.floor(width / (page.tileSize + Kirigami.Units.gridUnit * 4))))
                cellHeight: page.listView ? Math.max(Kirigami.Units.gridUnit * 3, Math.round(page.tileSize * 0.7) + Kirigami.Units.largeSpacing * 2)
                                          : Math.round(page.tileSize + Kirigami.Units.gridUnit * 3.4)
                model: page.activeItems
                QQC2.ScrollBar.vertical: PlasmaComponents.ScrollBar {}
            }
        }

        ColumnLayout {
            visible: page.letters.length > 4
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.preferredWidth: Kirigami.Units.gridUnit * 1.4
            spacing: 0
            Repeater {
                model: page.letters
                MouseArea {
                    id: letter
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.maximumHeight: Kirigami.Units.gridUnit * 1.3
                    hoverEnabled: modelData.row >= 0
                    enabled: modelData.row >= 0
                    onClicked: page.jump(modelData.row)
                    Rectangle {
                        anchors.centerIn: parent
                        width: Math.min(parent.width, parent.height)
                        height: width
                        radius: width / 2
                        color: letter.containsMouse ? launcher.selectedFill : "transparent"
                    }
                    PlasmaComponents.Label {
                        anchors.centerIn: parent
                        text: letter.modelData.key
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        font.weight: Font.DemiBold
                        opacity: letter.modelData.row < 0 ? 0.18 : letter.containsMouse ? 1 : 0.55
                    }
                }
            }
            Item { Layout.fillHeight: true }
        }
    }
}
