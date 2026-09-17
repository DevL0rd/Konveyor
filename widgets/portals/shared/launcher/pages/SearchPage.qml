import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQml.Models
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import "../lib/Highlight.js" as Highlight
import ".."

PopScroll {
    id: page

    readonly property string term: launcher.term
    readonly property string mode: launcher.mode
    readonly property int rowWidth: Kirigami.Units.gridUnit * 19
    readonly property int groupCount: launcherData.runner.count
    property int groupsRevision: 0

    readonly property var order: {
        const known = ["answer", "apps", "games", "windows", "settings", "files", "friends", "commands", "other"]
        const wanted = String(Plasmoid.configuration.searchOrder || "").split(",").map(key => key.trim()).filter(key => known.indexOf(key) >= 0)
        for (const key of known) {
            if (wanted.indexOf(key) < 0)
                wanted.push(key)
        }
        return wanted
    }

    function classify(name) {
        const text = String(name || "").toLowerCase()
        if (text.indexOf("application") >= 0)
            return "apps"
        if (text.indexOf("calculat") >= 0 || text.indexOf("unit conver") >= 0 || text.indexOf("date and time") >= 0 || text.indexOf("dictionary") >= 0)
            return "answer"
        if (text.indexOf("window") >= 0)
            return "windows"
        if (text.indexOf("setting") >= 0)
            return "settings"
        if (text.indexOf("place") >= 0 || text.indexOf("recent") >= 0 || text.indexOf("file") >= 0 || text.indexOf("location") >= 0 || text.indexOf("desktop search") >= 0 || text.indexOf("document") >= 0)
            return "files"
        if (text.indexOf("command") >= 0 || text.indexOf("shell") >= 0 || text.indexOf("terminal") >= 0)
            return "commands"
        return "other"
    }
    readonly property var groupsByKind: {
        groupsRevision
        const kinds = {}
        for (let row = 0; row < groupCount; ++row) {
            const group = launcherData.runner.modelForRow(row)
            const kind = group ? classify(group.name) : "other"
            if (!kinds[kind])
                kinds[kind] = []
            kinds[kind].push(row)
        }
        return kinds
    }
    readonly property var appGroup: {
        const rows = groupsByKind.apps || []
        return rows.length > 0 && (mode === "all" || mode === "apps") ? launcherData.runner.modelForRow(rows[0]) : null
    }

    readonly property var gameMatches: (mode === "all" || mode === "games") && term !== "" && launcherData.gamesEnabled
        ? launcherData.games.filter(game => Highlight.matches(game.name, term)).map(game => {
            const at = String(game.name).toLowerCase().indexOf(term.toLowerCase())
            return { game: game, score: at < 0 ? 1000 : at }
        }).sort((a, b) => (a.score - b.score) || (b.game.last - a.game.last)).map(entry => entry.game).slice(0, mode === "games" ? 40 : 8) : []
    readonly property var friendMatches: (mode === "all" || mode === "friends") && term !== "" && launcherData.friendsEnabled
        ? launcherData.friends.filter(friend => Highlight.matches(friend.name, term) || Highlight.matches(friend.game, term)).slice(0, mode === "friends" ? 40 : 6) : []
    readonly property bool showPackages: (mode === "all" || mode === "packages") && launcherData.packagesEnabled
    readonly property var shortcutMatches: mode === "all" && term.length >= 2 ? launcherData.shortcutMatches(term, 6) : []
    onShortcutMatchesChanged: Qt.callLater(rebuildSections)

    property var appIds: []
    function collectApps() {
        const ids = []
        const source = appsProbe
        for (let i = 0; i < source.count; ++i) {
            const object = source.objectAt(i)
            ids.push(object ? object.favoriteId : "")
        }
        appIds = ids
    }
    Instantiator {
        id: appsProbe
        model: page.appGroup
        delegate: QtObject {
            required property var model
            readonly property string favoriteId: model.favoriteId || ""
        }
        onObjectAdded: Qt.callLater(page.collectApps)
        onObjectRemoved: Qt.callLater(page.collectApps)
    }
    onAppGroupChanged: Qt.callLater(collectApps)

    readonly property var hero: {
        appIds
        const learned = launcherData.learnedFor(term)
        for (const key of learned) {
            if (key.startsWith("game:")) {
                const game = gameMatches.find(entry => "game:" + entry.id === key)
                if (game)
                    return { kind: "game", game: game }
            } else {
                const row = appIds.indexOf(key)
                if (row >= 0 && !launcherData.isHidden(key)) {
                    const game = launcherData.steamGameForApp(key)
                    return game ? { kind: "game", game: game, appRow: row } : { kind: "app", row: row }
                }
            }
        }
        for (let row = 0; row < appIds.length; ++row) {
            if (launcherData.isHidden(appIds[row]))
                continue
            const game = launcherData.steamGameForApp(appIds[row])
            return game ? { kind: "game", game: game, appRow: row } : { kind: "app", row: row }
        }
        if (gameMatches.length > 0)
            return { kind: "game", game: gameMatches[0] }
        return { kind: "none" }
    }
    readonly property var gameRows: {
        const heroId = hero.kind === "game" ? hero.game.id : ""
        const list = gameMatches.filter(game => game.id !== heroId && (game.appid !== "" || appIds.every(id => launcherData.desktopKey(id) !== game.id)))
        for (const id of appIds) {
            const game = launcherData.steamGameForApp(id)
            if (game && game.id !== heroId && !list.some(entry => entry.id === game.id))
                list.push(game)
        }
        return list
    }

    property var sections: []
    property int totalResults: 0

    function rebuildSections() {
        const list = [heroApp, heroGame]
        for (let i = 0; i < slots.count; ++i) {
            const slot = slots.itemAt(i)
            if (slot && slot.item && slot.item.hasContent)
                list.push.apply(list, slot.item.grids ? slot.item.grids() : [])
        }
        list.push(shortcutResults.grid)
        list.push(packagesResults.grid)
        let total = 0
        for (const grid of list)
            total += grid && grid.visible ? grid.count : 0
        sections = list
        totalResults = total
        launcher.ensureSelection()
    }
    onTermChanged: {
        if (term !== "")
            launcherData.ensureShortcuts()
        Qt.callLater(rebuildSections)
    }
    onHeroChanged: {
        pickHeroRow()
        Qt.callLater(rebuildSections)
    }
    onGameRowsChanged: Qt.callLater(rebuildSections)
    onFriendMatchesChanged: Qt.callLater(rebuildSections)
    onGroupCountChanged: {
        groupsRevision++
        Qt.callLater(rebuildSections)
    }
    Connections {
        target: launcherData
        function onPackagesChanged() { Qt.callLater(page.rebuildSections) }
    }
    Connections {
        target: launcherData.runner
        function onQueryFinished() {
            page.groupsRevision++
            Qt.callLater(page.rebuildSections)
        }
    }

    component ResultGroup: ColumnLayout {
        id: group
        property string title
        property string trailing: resultGrid.count + ""
        property alias grid: resultGrid
        property alias model: resultGrid.model
        property alias delegate: resultGrid.delegate
        property alias cellHeight: resultGrid.cellHeight
        property alias cellWidth: resultGrid.cellWidth
        property bool busy: false
        readonly property bool hasContent: resultGrid.count > 0 || busy
        visible: hasContent
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        SectionHeader {
            title: group.title
            trailing: group.busy && resultGrid.count === 0 ? i18n("searching…") : group.trailing
        }
        TileGrid {
            id: resultGrid
            visible: count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            cellWidth: Math.floor(width / Math.max(1, Math.floor(width / page.rowWidth)))
            cellHeight: Kirigami.Units.gridUnit * 3.2
            onCountChanged: Qt.callLater(page.rebuildSections)
        }
    }

    component HeroCard: Item {
        id: heroCard
        property var iconSource
        property string label
        property string subtitle
        property string kind
        property bool selected: false
        property var actions: []
        property var game: null
        signal clicked()
        signal rightClicked()
        signal hovered()

        Rectangle {
            anchors.fill: parent
            anchors.margins: 2
            radius: Kirigami.Units.cornerRadius * 3
            color: heroCard.selected ? launcher.selectedFill : heroMouse.containsMouse ? launcher.hoverFill : launcher.well
            border.width: 1
            border.color: heroCard.selected ? launcher.selectedLine : launcher.hairline
        }
        MouseArea {
            id: heroMouse
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onEntered: heroCard.hovered()
            onClicked: function(event) {
                if (event.button === Qt.RightButton)
                    heroCard.rightClicked()
                else
                    heroCard.clicked()
            }
        }
        RowLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing * 1.5
            spacing: Kirigami.Units.largeSpacing * 2
            GameArt {
                visible: heroCard.game !== null
                Layout.fillHeight: true
                Layout.preferredWidth: Math.round(height / 0.4667)
                game: heroCard.game || ({})
                wide: true
                radius: Kirigami.Units.cornerRadius * 2
            }
            Kirigami.Icon {
                visible: heroCard.game === null
                Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                source: heroCard.iconSource
                fallback: "application-x-executable"
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing * 0.5
                PlasmaComponents.Label {
                    text: heroCard.kind
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    font.weight: Font.DemiBold
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 0.6
                    opacity: 0.5
                }
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    text: heroCard.label
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.6
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    visible: text !== ""
                    text: heroCard.subtitle
                    opacity: 0.6
                    elide: Text.ElideRight
                }
            }
            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                Repeater {
                    model: heroCard.actions
                    PlasmaComponents.Button {
                        required property var modelData
                        required property int index
                        text: modelData.text
                        icon.name: modelData.icon
                        highlighted: index === 0
                        onClicked: modelData.run()
                    }
                }
            }
        }
    }

    SectionHeader {
        visible: heroApp.visible || heroGame.visible
        title: i18n("Best match")
        trailing: launcherData.learnedFor(page.term).length > 0 ? i18n("based on what you open") : ""
    }

    DelegateModel {
        id: heroAppModel
        model: page.hero.kind === "app" ? page.appGroup : null
        groups: DelegateModelGroup {
            name: "hero"
            includeByDefault: false
        }
        filterOnGroup: "hero"
        items.onChanged: page.pickHeroRow()
        delegate: HeroCard {
            id: appHero
            required property var model
            readonly property int sourceRow: DelegateModel.itemsIndex
            readonly property var grid: GridView.view
            readonly property string favoriteId: model.favoriteId || ""
            width: grid ? grid.cellWidth : 0
            height: grid ? grid.cellHeight : 0
            iconSource: model.decoration
            label: model.display || ""
            subtitle: model.description || ""
            kind: i18n("Application")
            selected: GridView.isCurrentItem && !!grid && grid.sectionActive
            actions: {
                const list = [{ text: i18n("Open"), icon: "system-run", run: () => appHero.activate() }]
                if (favoriteId !== "")
                    list.push({ text: launcher.isPinned(favoriteId) ? i18n("Unpin") : i18n("Pin"), icon: "window-pin", run: () => launcher.togglePin(favoriteId) })
                list.push({ text: i18n("More"), icon: "overflow-menu", run: () => appHero.openMenu() })
                return list
            }
            function activate() {
                launcher.trigger(page.appGroup, sourceRow, favoriteId)
            }
            function openMenu() {
                launcher.openMenu(launcher.kickerEntries(page.appGroup, sourceRow, model.hasActionList ? model.actionList : [], favoriteId), appHero)
            }
            onHovered: launcher.select(grid, 0)
            onClicked: activate()
            onRightClicked: {
                launcher.select(grid, 0)
                openMenu()
            }
        }
    }
    function pickHeroRow() {
        const items = heroAppModel.items
        const want = page.hero.kind === "app" ? page.hero.row : -1
        for (let i = 0; i < items.count; ++i) {
            const inHero = items.get(i).inHero
            if (i === want && !inHero)
                items.addGroups(i, 1, "hero")
            else if (i !== want && inHero)
                items.removeGroups(i, 1, "hero")
        }
    }

    TileGrid {
        id: heroApp
        visible: page.hero.kind === "app" && count > 0
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        limit: 1
        cellWidth: width
        cellHeight: Kirigami.Units.gridUnit * 5.4
        model: heroAppModel
        onCountChanged: Qt.callLater(page.rebuildSections)
    }

    TileGrid {
        id: heroGame
        visible: page.hero.kind === "game"
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        limit: 1
        cellWidth: width
        cellHeight: Kirigami.Units.gridUnit * 6
        model: page.hero.kind === "game" ? [page.hero.game] : []
        delegate: HeroCard {
            id: gameHero
            required property int index
            required property var modelData
            readonly property var grid: GridView.view
            readonly property var playing: launcherData.friendsFor(modelData)
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
                launcher.remember("game:" + modelData.id)
                launcherData.launchGame(modelData)
                root.hide()
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
        onCountChanged: Qt.callLater(page.rebuildSections)
    }

    Component {
        id: appsSlot
        ResultGroup {
            id: appsGroup
            function grids() {
                return [grid]
            }
            title: i18n("Applications")
            model: DelegateModel {
                id: appRows
                model: page.appGroup
                groups: DelegateModelGroup {
                    name: "shown"
                    includeByDefault: false
                }
                filterOnGroup: "shown"
                items.onChanged: appsGroup.filterRows()
                delegate: KickerRow {
                    sourceModel: page.appGroup
                    sourceIndex: DelegateModel.itemsIndex
                    function activate() {
                        launcher.trigger(page.appGroup, DelegateModel.itemsIndex, favoriteId)
                    }
                }
            }
            function filterRows() {
                const items = appRows.items
                const heroRow = page.hero.kind === "app" ? page.hero.row : page.hero.appRow !== undefined ? page.hero.appRow : -1
                for (let i = 0; i < items.count; ++i) {
                    const entry = items.get(i)
                    const id = entry.model.favoriteId || ""
                    const keep = i !== heroRow && !launcherData.isHidden(id) && launcherData.steamGameForApp(id) === null
                    if (keep && !entry.inShown)
                        items.addGroups(i, 1, "shown")
                    else if (!keep && entry.inShown)
                        items.removeGroups(i, 1, "shown")
                }
            }
            Connections {
                target: page
                function onHeroChanged() { appsGroup.filterRows() }
            }
        }
    }

    Component {
        id: gamesSlot
        ResultGroup {
            function grids() {
                return [grid]
            }
            title: i18n("Games")
            cellHeight: Kirigami.Units.gridUnit * 3.6
            model: page.gameRows
            delegate: RowTile {
                id: gameRow
                required property int index
                required property var modelData
                readonly property var grid: GridView.view
                readonly property var playing: launcherData.friendsFor(modelData)
                width: grid.cellWidth
                height: grid.cellHeight
                game: modelData.appid ? modelData : null
                iconSource: modelData.icon || "applications-games"
                label: modelData.name
                query: page.term
                subtitle: playing.length > 0 ? i18np("%1 friend playing", "%1 friends playing", playing.length) : modelData.last > 0 ? i18n("Played %1", launcherData.relativeTime(modelData.last)) : modelData.appid ? i18n("Steam game") : i18n("Game")
                subtitleColor: playing.length > 0 ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
                selected: GridView.isCurrentItem && grid.sectionActive
                function activate() {
                    launcher.remember("game:" + modelData.id)
                    launcherData.launchGame(modelData)
                    root.hide()
                }
                function openMenu() {
                    launcher.openMenu(launcher.gameEntries(modelData), gameRow)
                }
                onHovered: launcher.select(grid, index)
                onClicked: activate()
                onRightClicked: {
                    launcher.select(grid, index)
                    openMenu()
                }
            }
        }
    }

    Component {
        id: friendsSlot
        ResultGroup {
            function grids() {
                return [grid]
            }
            title: i18n("Friends")
            model: page.friendMatches
            delegate: FriendRow {}
        }
    }

    Component {
        id: runnerSlot
        ColumnLayout {
            id: runnerKind
            property string kind
            property int contentCount: 0
            readonly property bool hasContent: contentCount > 0
            function recount() {
                let total = 0
                for (let i = 0; i < kindGroups.count; ++i) {
                    const group = kindGroups.itemAt(i)
                    if (group)
                        total += group.grid.count
                }
                contentCount = total
            }
            spacing: Kirigami.Units.largeSpacing * 1.5
            function grids() {
                const list = []
                for (let i = 0; i < kindGroups.count; ++i) {
                    const group = kindGroups.itemAt(i)
                    if (group)
                        list.push(group.grid)
                }
                return list
            }
            Repeater {
                id: kindGroups
                model: page.groupsByKind[runnerKind.kind] || []
                delegate: ResultGroup {
                    required property var modelData
                    readonly property var runnerGroup: launcherData.runner.modelForRow(modelData)
                    Connections {
                        target: grid
                        function onCountChanged() { runnerKind.recount() }
                    }
                    title: runnerKind.kind === "answer" ? i18n("Answer") : runnerGroup ? runnerGroup.name : ""
                    trailing: runnerKind.kind === "answer" ? "" : grid.count + ""
                    cellWidth: runnerKind.kind === "answer" ? width : Math.floor(width / Math.max(1, Math.floor(width / page.rowWidth)))
                    cellHeight: Kirigami.Units.gridUnit * (runnerKind.kind === "answer" ? 3.8 : 3.2)
                    model: runnerGroup
                    delegate: KickerRow {
                        id: runnerRow
                        emphasize: runnerKind.kind === "answer"
                        subtitle: runnerKind.kind === "answer" ? page.term + " =" : (model.description || "")
                        trailing: runnerKind.kind === "answer" ? i18n("Enter copies the result") : ""
                        function activate() {
                            if (runnerKind.kind === "answer") {
                                launcherData.copyText(model.display || "")
                                root.hide()
                                return
                            }
                            launcher.trigger(sourceModel, sourceIndex, "")
                        }
                    }
                }
                onItemAdded: {
                    runnerKind.recount()
                    Qt.callLater(page.rebuildSections)
                }
                onItemRemoved: {
                    runnerKind.recount()
                    Qt.callLater(page.rebuildSections)
                }
            }
        }
    }

    Repeater {
        id: slots
        model: page.order
        delegate: Loader {
            id: slot
            required property string modelData
            Layout.fillWidth: true
            Layout.preferredHeight: item && item.hasContent ? item.implicitHeight : 0
            visible: item !== null && item.hasContent
            active: modelData !== "apps" || page.appGroup !== null
            sourceComponent: modelData === "apps" ? appsSlot : modelData === "games" ? gamesSlot : modelData === "friends" ? friendsSlot : runnerSlot
            onLoaded: {
                item.width = Qt.binding(() => slot.width)
                if (sourceComponent === runnerSlot)
                    item.kind = Qt.binding(() => modelData)
                Qt.callLater(page.rebuildSections)
            }
        }
    }

    ResultGroup {
        id: shortcutResults
        title: i18n("Shortcuts")
        model: page.shortcutMatches
        delegate: RowTile {
            id: shortcutRow
            required property int index
            required property var modelData
            readonly property var grid: GridView.view
            width: grid.cellWidth
            height: grid.cellHeight
            iconSource: "input-keyboard-symbolic"
            monochrome: true
            label: modelData.action
            query: page.term
            subtitle: modelData.section
            trailing: launcherData.keyText(modelData.keys[0])
            selected: GridView.isCurrentItem && grid.sectionActive
            function activate() {
                launcherData.shortcutFocus = modelData.action
                launcher.goToPage("shortcuts")
            }
            function openMenu() {
                activate()
            }
            onHovered: launcher.select(grid, index)
            onClicked: activate()
        }
    }

    ResultGroup {
        id: packagesResults
        title: i18n("Install with Shelly")
        visible: page.showPackages && (grid.count > 0 || (busy && (page.totalResults > 0 || page.mode === "packages")))
        busy: launcherData.packagesBusy
        model: page.showPackages ? launcherData.packages : []
        delegate: RowTile {
            id: packageRow
            required property int index
            required property var modelData
            readonly property var grid: GridView.view
            width: grid.cellWidth
            height: grid.cellHeight
            iconSource: "package-x-generic-symbolic"
            monochrome: true
            label: modelData.name
            query: page.term
            subtitle: modelData.description
            trailing: modelData.source === "aur" ? i18np("AUR · %1 vote", "AUR · %1 votes", modelData.votes) : modelData.repo
            selected: GridView.isCurrentItem && grid.sectionActive
            function activate() {
                launcherData.installPackage(modelData)
                root.hide()
            }
            function openMenu() {
                launcher.openMenu(launcher.packageEntries(modelData), packageRow)
            }
            onHovered: launcher.select(grid, index)
            onClicked: activate()
            onRightClicked: {
                launcher.select(grid, index)
                openMenu()
            }
        }
    }

    RowLayout {
        visible: page.totalResults === 0 && page.term !== "" && (launcherData.runner.querying || (launcherData.packagesBusy && page.showPackages))
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.gridUnit * 2
        spacing: Kirigami.Units.largeSpacing
        Kirigami.Icon {
            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
            Layout.preferredHeight: Layout.preferredWidth
            source: "search-symbolic"
            color: launcher.ink
            isMask: true
            opacity: 0.4
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            text: launcherData.packagesBusy && page.showPackages ? i18n("Nothing here yet — still checking files and Shelly packages…") : i18n("Searching…")
            opacity: 0.55
        }
    }

    ColumnLayout {
        visible: page.totalResults === 0 && !launcherData.runner.querying && page.term !== "" && !(launcherData.packagesBusy && page.showPackages)
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.gridUnit * 4
        spacing: Kirigami.Units.largeSpacing
        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
            Layout.preferredHeight: Layout.preferredWidth
            source: "edit-find-symbolic"
            color: launcher.ink
            isMask: true
            opacity: 0.35
        }
        PlasmaComponents.Label {
            Layout.alignment: Qt.AlignHCenter
            text: i18n("Nothing found for “%1”", page.term)
            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.3
            font.weight: Font.DemiBold
        }
        PlasmaComponents.Label {
            Layout.alignment: Qt.AlignHCenter
            text: i18n("Narrow it down with a prefix")
            opacity: 0.55
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.smallSpacing
            Repeater {
                model: [
                    { prefix: "g ", label: i18n("Games") },
                    { prefix: "a ", label: i18n("Apps") },
                    { prefix: "f ", label: i18n("Files") },
                    { prefix: "s ", label: i18n("Packages") },
                    { prefix: "@", label: i18n("Friends") },
                    { prefix: "=", label: i18n("Math") },
                    { prefix: ">", label: i18n("Command") }
                ]
                MouseArea {
                    id: chip
                    required property var modelData
                    implicitWidth: chipRow.implicitWidth + Kirigami.Units.largeSpacing * 2
                    implicitHeight: Kirigami.Units.gridUnit * 1.9
                    hoverEnabled: true
                    onClicked: launcher.setQuery(modelData.prefix + page.term)
                    Rectangle {
                        anchors.fill: parent
                        radius: height / 2
                        color: chip.containsMouse ? launcher.selectedFill : launcher.well
                        border.width: 1
                        border.color: launcher.hairline
                    }
                    RowLayout {
                        id: chipRow
                        anchors.centerIn: parent
                        spacing: Kirigami.Units.smallSpacing
                        PlasmaComponents.Label {
                            text: chip.modelData.prefix.trim()
                            font.family: "monospace"
                            font.weight: Font.DemiBold
                        }
                        PlasmaComponents.Label {
                            text: chip.modelData.label
                            opacity: 0.7
                        }
                    }
                }
            }
        }
    }

    Item {
        Layout.fillHeight: true
    }
}
