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
        if (page.sort === "installed")
            return entry.model.isNewlyInstalled === true ? 0 : 1
        return 0
    }
    function arrange() {
        const items = shownItems.items
        for (let i = 0; i < items.count; ++i) {
            const entry = items.get(i)
            const keep = !launcherData.isHidden(entry.model.favoriteId || "")
            if (keep && !entry.inShown)
                items.addGroups(i, 1, "shown")
            else if (!keep && entry.inShown)
                items.removeGroups(i, 1, "shown")
        }
        const group = shownGroup
        if (page.sort !== "name") {
            const order = []
            for (let i = 0; i < group.count; ++i)
                order.push({ rank: page.rankOf(group.get(i)), label: String(group.get(i).model.display || "").toLowerCase(), row: group.get(i).model.index })
            const sorted = order.slice().sort((a, b) => (a.rank - b.rank) || a.label.localeCompare(b.label))
            for (let target = 0; target < sorted.length; ++target) {
                let from = -1
                for (let i = target; i < group.count; ++i) {
                    if (group.get(i).model.index === sorted[target].row) {
                        from = i
                        break
                    }
                }
                if (from > target)
                    group.move(from, target, 1)
            }
        }
        const found = []
        if (page.sort === "name") {
            for (let i = 0; i < group.count; ++i) {
                const label = String(group.get(i).model.display || "")
                const first = label.charAt(0).toUpperCase()
                const key = /[A-Z]/.test(first) ? first : "#"
                if (found.length === 0 || found[found.length - 1].key !== key)
                    found.push({ key: key, row: i })
            }
        }
        page.letters = found
    }
    Timer {
        id: arrangeTimer
        interval: 0
        onTriggered: page.arrange()
    }
    onSortChanged: arrangeTimer.restart()
    Connections {
        target: Plasmoid.configuration
        function onHiddenAppsChanged() { arrangeTimer.restart() }
    }
    Connections {
        target: launcherData
        function onRecentRankChanged() { if (page.sort === "recent") arrangeTimer.restart() }
    }

    DelegateModel {
        id: shownItems
        model: page.categoryModel
        groups: DelegateModelGroup {
            id: shownGroup
            name: "shown"
            includeByDefault: false
        }
        filterOnGroup: "shown"
        items.onChanged: arrangeTimer.restart()
        delegate: Loader {
            id: cell
            required property var model
            required property int index
            readonly property var view: GridView.view
            width: view ? view.cellWidth : 0
            height: view ? view.cellHeight : 0
            sourceComponent: page.listView ? rowComponent : tileComponent
            readonly property bool isCurrent: GridView.isCurrentItem
            readonly property string favoriteId: model.favoriteId || ""
            onLoaded: {
                item.width = Qt.binding(() => cell.width)
                item.height = Qt.binding(() => cell.height)
            }
            function activate() { if (item) item.activate() }
            function openMenu() { if (item) item.openMenu() }
            Component {
                id: tileComponent
                Tile {
                    id: appTile
                    readonly property string favoriteId: cell.model.favoriteId || ""
                    iconSize: page.tileSize
                    iconSource: cell.model.decoration
                    label: cell.model.display || ""
                    badge: cell.model.isNewlyInstalled === true
                    game: launcherData.gameForApp(favoriteId)
                    selected: cell.isCurrent && !!cell.view && cell.view.sectionActive
                    function activate() {
                        launcher.trigger(page.categoryModel, cell.model.index, favoriteId)
                    }
                    function openMenu() {
                        launcher.openMenu(launcher.kickerEntries(page.categoryModel, cell.model.index, cell.model.hasActionList ? cell.model.actionList : [], favoriteId), appTile)
                    }
                    onHovered: launcher.select(cell.view, cell.index)
                    onClicked: activate()
                    onRightClicked: {
                        launcher.select(cell.view, cell.index)
                        openMenu()
                    }
                }
            }
            Component {
                id: rowComponent
                RowTile {
                    id: appRow
                    readonly property string favoriteId: cell.model.favoriteId || ""
                    readonly property var gameEntry: launcherData.gameForApp(favoriteId)
                    iconSource: cell.model.decoration
                    iconSize: Math.max(Kirigami.Units.iconSizes.medium, Math.round(page.tileSize * 0.7))
                    game: gameEntry && gameEntry.appid ? gameEntry : null
                    label: cell.model.display || ""
                    subtitle: cell.model.description || ""
                    trailing: cell.model.isNewlyInstalled === true ? i18n("New") : ""
                    selected: cell.isCurrent && !!cell.view && cell.view.sectionActive
                    function activate() {
                        launcher.trigger(page.categoryModel, cell.model.index, favoriteId)
                    }
                    function openMenu() {
                        launcher.openMenu(launcher.kickerEntries(page.categoryModel, cell.model.index, cell.model.hasActionList ? cell.model.actionList : [], favoriteId), appRow)
                    }
                    onHovered: launcher.select(cell.view, cell.index)
                    onClicked: activate()
                    onRightClicked: {
                        launcher.select(cell.view, cell.index)
                        openMenu()
                    }
                }
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
            text: i18np("%1 app", "%1 apps", shownGroup.count)
            opacity: 0.5
        }

        Segment {
            id: sortButton
            text: i18n("Sort: %1", (page.sortDefs.find(entry => entry.key === page.sort) || page.sortDefs[0]).label)
            iconName: "view-sort-symbolic"
            onClicked: launcher.openMenu(page.sortDefs.map(entry => ({ text: entry.label, icon: entry.key === page.sort ? "checkmark" : "", run: () => { Plasmoid.configuration.appsSort = entry.key; Qt.callLater(launcher.resetSelection) } })), sortButton)
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
                model: shownItems
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
                    hoverEnabled: true
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
                        opacity: letter.containsMouse ? 1 : 0.5
                    }
                }
            }
            Item { Layout.fillHeight: true }
        }
    }
}
