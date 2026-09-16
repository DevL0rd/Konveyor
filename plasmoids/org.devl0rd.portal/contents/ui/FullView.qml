import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "lib"

Item {
    id: full

    Layout.minimumWidth: Kirigami.Units.gridUnit * 18
    Layout.minimumHeight: Kirigami.Units.gridUnit * 16
    Layout.preferredWidth: Kirigami.Units.gridUnit * 36
    Layout.preferredHeight: Kirigami.Units.gridUnit * 32

    readonly property var tabDefs: [
        { key: "fav", label: i18n("Favourites"), icon: "favorite" },
        { key: "apps", label: i18n("Applications"), icon: "applications-all" },
        { key: "games", label: i18n("Games"), icon: "applications-games" }
    ]

    Loader {
        id: loader
        anchors.fill: parent
        active: root.popupAlive
        sourceComponent: shellComponent
        onLoaded: if (root.expanded) item.focusSearch()
    }

    Connections {
        target: root
        function onExpandedChanged() {
            if (root.expanded && loader.item)
                loader.item.focusSearch()
        }
    }

    Component {
        id: shellComponent

        PopupShell {
            id: shell

            readonly property bool universalSearch: root.favActive && searchText.trim() !== ""
            readonly property var allAppsModel: root.appModelFor(root.allAppsCategory)
            readonly property int appCount: allAppsModel ? allAppsModel.count : 0

            anchors.fill: parent
            icon: root.panelIcon
            title: i18n("App Portal")
            subtitle: {
                const parts = [i18np("%1 app", "%1 apps", appCount)]
                if (root.games.length > 0)
                    parts.push(i18np("%1 game", "%1 games", root.games.length))
                if (root.friendsPlaying > 0)
                    parts.push(i18np("%1 friend in game", "%1 friends in game", root.friendsPlaying))
                return parts.join("  ·  ")
            }
            statusVisible: root.friendsPlaying > 0
            statusColor: "#43a047"
            statusText: i18np("%1 friend in game", "%1 friends in game", root.friendsPlaying)
            searchPlaceholder: root.gamesActive ? i18n("Search games…")
                             : root.appsActive ? i18n("Search %1…", root.allAppsActive ? i18n("applications") : root.currentCat.label)
                             : i18n("Search apps, favourites and games…")
            showTabs: true
            tabs: full.tabDefs.map(tab => ({
                key: tab.key, label: tab.label, icon: tab.icon,
                badge: tab.key === "fav" && root.favorites.length > 0 ? root.favorites.length + ""
                     : tab.key === "games" && root.games.length > 0 ? root.games.length + "" : ""
            }))
            currentTab: Math.max(0, full.tabDefs.findIndex(tab => tab.key === root.tabKey))
            matchCount: universalSearch ? searchResults.count : -1
            onTabActivated: index => root.selectTab(full.tabDefs[index].key)
            onCloseRequested: root.expanded = false
            onSearchTextChanged: root.searchText = searchText
            onSearchNext: if (universalSearch) searchResults.move(1)
            onSearchPrevious: if (universalSearch) searchResults.move(-1)
            onSearchAccepted: {
                if (universalSearch)
                    searchResults.activateCurrent()
                else if (root.favActive)
                    favGrid.activateFirst()
                else if (root.appsActive)
                    appsPage.activateFirst()
                else if (root.gamesActive)
                    gamesView.activateFirst()
            }
            Component.onCompleted: if (root.searchText !== "") shell.searchText = root.searchText

            headerActions: [
                PlasmaComponents.ToolButton {
                    icon.name: root.iconFor(root.sortOptions, root.sortMode, "view-sort")
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Sort")
                    onClicked: sortMenu.popup()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                    QQC2.Menu {
                        id: sortMenu
                        Instantiator {
                            model: root.sortOptions
                            delegate: QQC2.MenuItem {
                                required property var modelData
                                text: modelData.label
                                icon.name: modelData.icon
                                checkable: true
                                checked: root.sortMode === modelData.id
                                onTriggered: {
                                    root.sortMode = modelData.id
                                    Plasmoid.configuration.defaultSort = modelData.id
                                }
                            }
                            onObjectAdded: function(i, o) { sortMenu.insertItem(i, o) }
                            onObjectRemoved: function(i, o) { sortMenu.removeItem(o) }
                        }
                        QQC2.MenuSeparator {
                            visible: root.gamesActive
                            height: visible ? implicitHeight : 0
                        }
                        QQC2.MenuItem {
                            text: i18n("Friends online only")
                            icon.name: "im-user"
                            visible: root.gamesActive
                            height: visible ? implicitHeight : 0
                            checkable: true
                            checked: Plasmoid.configuration.gamesFriendsOnly
                            onTriggered: Plasmoid.configuration.gamesFriendsOnly = checked
                        }
                    }
                },
                PlasmaComponents.ToolButton {
                    id: viewButton
                    icon.name: root.iconFor(root.viewOptions, root.currentViewMode, "view-list-icons")
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("View & zoom")
                    onClicked: viewPopup.open()
                    QQC2.ToolTip.visible: hovered && !viewPopup.visible
                    QQC2.ToolTip.text: text

                    QQC2.Popup {
                        id: viewPopup
                        y: viewButton.height
                        x: viewButton.width - width
                        padding: Kirigami.Units.largeSpacing
                        contentItem: ColumnLayout {
                            spacing: Kirigami.Units.smallSpacing
                            PlasmaComponents.Label {
                                text: i18n("View")
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                font.weight: Font.DemiBold
                                font.capitalization: Font.AllUppercase
                                opacity: 0.65
                            }
                            Repeater {
                                model: root.viewOptions
                                delegate: QQC2.RadioButton {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    text: modelData.label
                                    icon.name: modelData.icon
                                    checked: root.currentViewMode === modelData.id
                                    onClicked: root.setViewMode(modelData.id)
                                }
                            }
                            PlasmaComponents.Button {
                                visible: root.gamesActive
                                Layout.fillWidth: true
                                text: i18n("Refresh games")
                                icon.name: "view-refresh"
                                onClicked: root.reloadGames()
                            }
                            Kirigami.Separator { Layout.fillWidth: true }
                            RowLayout {
                                Layout.fillWidth: true
                                Kirigami.Icon {
                                    source: "zoom-out"
                                    Layout.preferredWidth: Kirigami.Units.iconSizes.small
                                    Layout.preferredHeight: Kirigami.Units.iconSizes.small
                                    opacity: 0.7
                                }
                                QQC2.Slider {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: Kirigami.Units.gridUnit * 8
                                    from: root.gamesActive ? 100 : 32
                                    to: root.gamesActive ? 320 : 160
                                    stepSize: root.gamesActive ? 10 : 8
                                    value: root.gamesActive ? Plasmoid.configuration.gameCardWidth : Plasmoid.configuration.iconSize
                                    onMoved: {
                                        if (root.gamesActive)
                                            Plasmoid.configuration.gameCardWidth = value
                                        else
                                            Plasmoid.configuration.iconSize = value
                                    }
                                }
                                Kirigami.Icon {
                                    source: "zoom-in"
                                    Layout.preferredWidth: Kirigami.Units.iconSizes.small
                                    Layout.preferredHeight: Kirigami.Units.iconSizes.small
                                    opacity: 0.7
                                }
                            }
                        }
                    }
                },
                PlasmaComponents.ToolButton {
                    icon.name: "configure"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Configure…")
                    onClicked: Plasmoid.internalAction("configure").trigger()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                }
            ]

            ColumnLayout {
                anchors.fill: parent
                spacing: Kirigami.Units.largeSpacing

                CategoryBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: implicitHeight
                    visible: root.appsActive
                    categories: root.appCategories
                    current: root.selectedLabel
                    onPicked: label => root.selectCategory(label)
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    SearchResults {
                        id: searchResults
                        anchors.fill: parent
                        visible: shell.universalSearch
                        appModel: shell.allAppsModel
                        query: shell.universalSearch ? shell.searchText : ""
                    }

                    FavGrid {
                        id: favGrid
                        anchors.fill: parent
                        visible: root.favActive && !shell.universalSearch
                        viewMode: root.appViewMode
                        favorites: root.favorites
                        searchText: ""
                        onLaunched: root.launchAndClose()
                        onRemoveFav: function(resource) { root.toggleFavorite(resource, false) }
                    }

                    AppsPage {
                        id: appsPage
                        anchors.fill: parent
                        visible: root.appsActive
                        category: root.appsActive ? root.currentCat : null
                        allApps: root.allAppsActive
                        sections: root.appSectionCategories
                        favorites: root.favorites
                        favSet: root.favSet
                        viewMode: root.appViewMode
                        searchText: root.appsActive ? shell.searchText : ""
                        sortMode: root.sortMode
                        usage: root.usage
                    }

                    GamesView {
                        id: gamesView
                        anchors.fill: parent
                        visible: root.gamesActive
                        cardWidth: Plasmoid.configuration.gameCardWidth
                        viewMode: Plasmoid.configuration.gamesViewMode
                        searchText: root.gamesActive ? shell.searchText : ""
                        sortMode: root.sortMode
                        usage: root.usage
                        showTitles: Plasmoid.configuration.showGameTitles
                        friendsOnly: Plasmoid.configuration.gamesFriendsOnly
                    }

                    MouseArea {
                        anchors.fill: parent
                        z: 50
                        acceptedButtons: Qt.NoButton
                        onWheel: function(wheel) {
                            if (wheel.modifiers & Qt.ControlModifier) {
                                root.zoom(wheel.angleDelta.y > 0 ? 1 : -1)
                                wheel.accepted = true
                            } else if (root.gamesActive && gamesView.isCarousel) {
                                gamesView.browse(wheel.angleDelta.y > 0 ? -1 : 1)
                                wheel.accepted = true
                            } else {
                                wheel.accepted = false
                            }
                        }
                    }
                }
            }
        }
    }
}
