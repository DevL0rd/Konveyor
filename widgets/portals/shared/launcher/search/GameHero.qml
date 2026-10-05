import QtQuick

HeroCard {
    id: gameHero
    required property int index
    required property var modelData
    readonly property var grid: GridView.view
    readonly property var playing: launcherData.friendsFor(modelData)
    readonly property var sidebarEntry: launcherData.sidebarEntryForGame(modelData)
    width: grid.cellWidth
    height: grid.cellHeight
    label: modelData.name
    game: modelData
    subtitle: playing.length > 0 ? i18np("%1 friend playing now", "%1 friends playing now", playing.length)
            : modelData.last > 0 ? i18n("Played %1", launcherData.relativeTime(modelData.last)) : i18n("Not played yet")
    kind: i18n("Game")
    selected: GridView.isCurrentItem && grid.sectionActive
    actions: [
        { text: i18n("Play"), icon: "media-playback-start", run: () => gameHero.activate() },
        { text: i18n("More"), icon: "overflow-menu", run: () => gameHero.openMenu() }
    ]
    function activate() {
        launcher.launchGame(modelData)
    }
    function openMenu() {
        launcher.openMenu(launcher.gameEntries(modelData), gameHero)
    }
    onHovered: launcher.select(grid, index)
    onClicked: activate()
    onRightClicked: {
        launcher.select(grid, index)
        openMenu()
    }
}
