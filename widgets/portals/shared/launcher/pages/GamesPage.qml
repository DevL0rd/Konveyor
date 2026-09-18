import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import "../lib"
import ".."

ColumnLayout {
    id: page

    readonly property string view: Plasmoid.configuration.gamesView
    readonly property string sort: Plasmoid.configuration.gamesSort
    readonly property bool carousel: view === "carousel" || view === "coverflow"
    readonly property var sections: carousel ? [flow] : view === "list" ? [list] : [grid]
    property string filter: "all"

    readonly property var viewDefs: [
        { key: "grid", label: i18n("Grid"), icon: "view-grid-symbolic" },
        { key: "banner", label: i18n("Banners"), icon: "view-list-icons-symbolic" },
        { key: "list", label: i18n("List"), icon: "view-list-details-symbolic" },
        { key: "carousel", label: i18n("Carousel"), icon: "view-media-playlist-symbolic" },
        { key: "coverflow", label: i18n("Cover flow"), icon: "view-preview-symbolic" }
    ]
    readonly property var filters: [
        { key: "all", label: i18n("All") },
        { key: "played", label: i18n("Played") },
        { key: "friends", label: i18n("Friends playing") }
    ]
    readonly property var sortDefs: [
        { key: "recent", label: i18n("Last played") },
        { key: "name", label: i18n("Name") },
        { key: "friends", label: i18n("Friends playing") }
    ]
    readonly property var shown: {
        let list = launcherData.games
        if (filter === "played")
            list = list.filter(game => game.last > 0)
        else if (filter === "friends")
            list = list.filter(game => launcherData.friendsFor(game).length > 0)
        list = list.slice()
        const byName = (a, b) => String(a.name).toLowerCase().localeCompare(String(b.name).toLowerCase())
        if (sort === "name")
            list.sort(byName)
        else if (sort === "friends")
            list.sort((a, b) => (launcherData.friendsFor(b).length - launcherData.friendsFor(a).length) || (b.last - a.last) || byName(a, b))
        else
            list.sort((a, b) => (b.last - a.last) || byName(a, b))
        return list
    }

    spacing: Kirigami.Units.largeSpacing

    function cycle(forward) {
        const at = filters.findIndex(entry => entry.key === filter)
        filter = filters[(at + (forward ? 1 : -1) + filters.length) % filters.length].key
        Qt.callLater(launcher.resetSelection)
    }
    function setView(key) {
        Plasmoid.configuration.gamesView = key
        Qt.callLater(launcher.resetSelection)
    }
    function zoom(steps) {
        Plasmoid.configuration.gameCardSize = Math.max(6, Math.min(18, Plasmoid.configuration.gameCardSize + steps))
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing

        SegmentGroup {
            Repeater {
                model: page.filters
                Segment {
                    required property var modelData
                    text: modelData.key === "friends" && launcherData.friendsInGame > 0 ? modelData.label + "  " + launcherData.friendsInGame : modelData.label
                    current: page.filter === modelData.key
                    onClicked: {
                        page.filter = modelData.key
                        Qt.callLater(launcher.resetSelection)
                    }
                }
            }
        }

        PlasmaComponents.Label {
            visible: !launcher.compact
            text: i18np("%1 game", "%1 games", page.shown.length)
            opacity: 0.5
        }

        Item { Layout.fillWidth: true }

        Segment {
            id: sortButton
            iconOnly: launcher.compact
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
                            Plasmoid.configuration.gamesSort = modelData.key
                            Qt.callLater(launcher.resetSelection)
                        }
                    }
                    onObjectAdded: (index, object) => sortMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => sortMenu.removeItem(object)
                }
            }
        }

        SegmentGroup {
            visible: page.view !== "list" && !launcher.compact
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
            Repeater {
                model: page.viewDefs
                Segment {
                    required property var modelData
                    iconOnly: true
                    iconName: modelData.icon
                    text: modelData.label
                    current: page.view === modelData.key
                    onClicked: page.setView(modelData.key)
                }
            }
        }
    }

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
            visible: page.view === "grid" || page.view === "banner"
            scrolling: true
            wideCards: page.view === "banner"
            showTitles: page.view === "grid"
            cardSpacing: Kirigami.Units.largeSpacing * 0.8
            readonly property real target: Kirigami.Units.gridUnit * Plasmoid.configuration.gameCardSize * (wideCards ? 2.1 : 1)
            cellWidth: Math.floor(width / Math.max(1, Math.round(width / target)))
            cellHeight: Math.round((cellWidth - cardSpacing * 2) * (wideCards ? 0.4667 : 1.5) + cardSpacing * 2)
            model: visible ? page.shown : []
            delegate: GameTile {}
            QQC2.ScrollBar.vertical: PlasmaComponents.ScrollBar {}
        }

        TileGrid {
            id: list
            anchors.fill: parent
            visible: page.view === "list"
            scrolling: true
            cellWidth: width
            cellHeight: Kirigami.Units.gridUnit * 4.4
            model: visible ? page.shown : []
            QQC2.ScrollBar.vertical: PlasmaComponents.ScrollBar {}
            delegate: Item {
                id: row
                required property int index
                required property var modelData
                readonly property var grid: GridView.view
                readonly property var playing: launcherData.friendsFor(modelData)
                readonly property bool selected: GridView.isCurrentItem && grid.sectionActive
                readonly property var sidebarEntry: launcherData.sidebarEntryForGame(modelData)
                width: grid.cellWidth
                height: grid.cellHeight

                function activate() {
                    launcher.launchGame(modelData)
                }
                function openMenu() {
                    launcher.openMenu(launcher.gameEntries(modelData), row)
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 3
                    radius: Kirigami.Units.cornerRadius * 2.5
                    color: row.selected ? launcher.selectedFill : rowMouse.containsMouse ? launcher.hoverFill : "transparent"
                    border.width: row.selected ? 1 : 0
                    border.color: launcher.selectedLine
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.smallSpacing * 2
                    spacing: Kirigami.Units.largeSpacing * 1.5
                    GameArt {
                        Layout.fillHeight: true
                        Layout.preferredWidth: height / 0.4667
                        game: row.modelData
                        wide: true
                        radius: Kirigami.Units.cornerRadius * 1.5
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing * 0.5
                        PlasmaComponents.Label {
                            Layout.fillWidth: true
                            text: row.modelData.name
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                        PlasmaComponents.Label {
                            Layout.fillWidth: true
                            text: row.modelData.last > 0 ? i18n("Played %1", launcherData.relativeTime(row.modelData.last)) : i18n("Not played yet")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.55
                            elide: Text.ElideRight
                        }
                    }
                    RowLayout {
                        visible: row.playing.length > 0
                        spacing: Kirigami.Units.smallSpacing
                        Rectangle {
                            Layout.preferredWidth: Kirigami.Units.smallSpacing * 1.6
                            Layout.preferredHeight: Layout.preferredWidth
                            radius: width / 2
                            color: Kirigami.Theme.positiveTextColor
                        }
                        PlasmaComponents.Label {
                            text: i18np("%1 friend playing", "%1 friends playing", row.playing.length)
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.8
                        }
                    }
                    PlasmaComponents.Label {
                        visible: row.modelData.appid === ""
                        text: i18n("Not on Steam")
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.4
                        Layout.rightMargin: Kirigami.Units.largeSpacing
                    }
                }
                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onEntered: launcher.select(row.grid, row.index)
                    onPressAndHold: function(event) {
                        if (!launcher.touchMode) {
                            event.accepted = false
                            return
                        }
                        launcher.select(row.grid, row.index)
                        row.openMenu()
                    }
                    onClicked: function(event) {
                        launcher.select(row.grid, row.index)
                        if (event.button === Qt.RightButton)
                            row.openMenu()
                        else
                            row.activate()
                    }
                }
            }
        }

        Item {
            id: flow
            anchors.fill: parent
            visible: page.carousel

            property bool sectionActive: false
            readonly property int shownCount: page.carousel ? page.shown.length : 0
            readonly property int columns: 1
            readonly property bool scrolling: true
            property int currentIndex: -1

            onCurrentIndexChanged: if (currentIndex >= 0 && cover.currentIndex !== currentIndex) cover.jumpTo(currentIndex)
            Connections {
                target: cover
                function onCurrentIndexChanged() {
                    if (flow.currentIndex >= 0)
                        flow.currentIndex = cover.currentIndex
                }
            }

            function reset() {
                currentIndex = shownCount > 0 ? cover.currentIndex : -1
            }
            function move(dx, dy) {
                if (dx === 0 || shownCount === 0)
                    return false
                cover.browse(dx)
                return true
            }
            function enterFrom(fromBelow) {
                reset()
                return shownCount > 0
            }
            function itemAtIndex(index) {
                return null
            }
            function activate() {
                const game = page.shown[cover.currentIndex]
                if (game) {
                    launcher.launchGame(game)
                }
            }
            function openMenu() {
                const game = page.shown[cover.currentIndex]
                if (game)
                    launcher.openMenu(launcher.gameEntries(game), cover)
            }

            CoverFlow {
                id: cover
                anchors.fill: parent
                items: page.carousel ? page.shown : []
                friendsByAppid: launcherData.friendsByAppid
                tilt: page.view === "coverflow"
                shrink: page.view === "coverflow"
                cardWidth: Kirigami.Units.gridUnit * Plasmoid.configuration.gameCardSize * 1.2
                showTitles: false
                highlightCenter: flow.sectionActive
                launchOnCenterClick: true
                onLaunchRequested: function(game) {
                    launcher.launchGame(game)
                }
                onMenuRequested: function(game) {
                    launcher.openMenu(launcher.gameEntries(game), cover)
                }
            }

            PlasmaComponents.Label {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Kirigami.Units.largeSpacing
                visible: page.shown.length > 0
                text: page.shown[cover.currentIndex] ? page.shown[cover.currentIndex].name : ""
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.25
                font.weight: Font.DemiBold
            }
        }

        PlasmaExtras.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: page.shown.length === 0
            iconName: page.filter === "friends" ? "system-users-symbolic" : "input-gamepad-symbolic"
            text: page.filter === "friends" ? i18n("No friends are playing your games right now")
                : page.filter === "played" ? i18n("Nothing played yet") : i18n("No games found")
            explanation: page.filter !== "all" ? i18n("Tab switches to another filter") : ""
        }
    }
}
