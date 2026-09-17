import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import Qt5Compat.GraphicalEffects
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import "lib/Highlight.js" as Highlight

Item {
    id: results

    property string query
    property var appModel: null
    readonly property int count: list.count
    readonly property string needle: query.trim().toLowerCase()

    function move(step) {
        if (list.count === 0) return
        list.currentIndex = Math.max(0, Math.min(list.count - 1, list.currentIndex + step))
        list.positionViewAtIndex(list.currentIndex, ListView.Contain)
    }
    function activateCurrent() {
        if (list.count === 0) return
        activate(entries[Math.max(0, list.currentIndex)])
    }
    function activate(entry) {
        if (!entry) return
        if (entry.kind === "fav") {
            root.run(entry.fav.launch)
            root.launchAndClose()
        } else if (entry.kind === "app") {
            if (results.appModel && results.appModel.trigger(entry.row, "", null)) {
                root.recordLaunch(entry.url)
                root.launchAndClose()
            }
        } else if (entry.kind === "game") {
            root.launchGame(entry.game)
        }
    }

    property var apps: []
    function rebuildApps() {
        const arr = []
        for (let i = 0; i < appRows.count; i++) {
            const o = appRows.objectAt(i)
            if (o)
                arr.push({ row: o.row, name: o.name, icon: o.icon, url: o.url, favoriteId: o.favoriteId })
        }
        apps = arr
    }
    Instantiator {
        id: appRows
        model: results.appModel
        delegate: QtObject {
            required property var model
            required property int index
            readonly property int row: index
            readonly property string name: model.display || ""
            readonly property var icon: model.decoration
            readonly property string url: model.url || ""
            readonly property string favoriteId: model.favoriteId || ""
        }
        onObjectAdded: Qt.callLater(results.rebuildApps)
        onObjectRemoved: Qt.callLater(results.rebuildApps)
    }

    function score(name) {
        const lower = (name || "").toLowerCase()
        if (lower === needle) return 0
        if (lower.startsWith(needle)) return 1
        if (lower.split(/[\s\-_.:]+/).some(word => word.startsWith(needle))) return 2
        return lower.indexOf(needle) >= 0 ? 3 : -1
    }
    function lastUsed(key) { return root.usage[key] || 0 }
    function ranked(items, nameOf, keyOf) {
        return items.map(item => ({ item: item, s: score(nameOf(item)) }))
            .filter(e => e.s >= 0)
            .sort((a, b) => a.s - b.s || lastUsed(keyOf(b.item)) - lastUsed(keyOf(a.item)) || nameOf(a.item).localeCompare(nameOf(b.item)))
            .map(e => e.item)
    }

    readonly property var entries: {
        if (needle === "") return []
        const out = []
        const favIds = {}
        for (const fav of ranked(root.favorites, f => f.name || "", f => "file:///usr/share/applications/" + (f.id || ""))) {
            favIds[root.favKey(fav.id)] = true
            out.push({ kind: "fav", section: i18n("Favourites"), name: fav.name || "", icon: fav.icon || "application-x-executable", fav: fav })
        }
        for (const app of ranked(apps, a => a.name, a => a.url)) {
            if (app.favoriteId && favIds[root.favKey(app.favoriteId)]) continue
            if (root.isHidden(app.favoriteId || app.url)) continue
            out.push({ kind: "app", section: i18n("Applications"), name: app.name, icon: app.icon, row: app.row, url: app.url })
        }
        for (const game of ranked(root.games, g => g.name || "", g => g.id)) {
            out.push({ kind: "game", section: i18n("Games"), name: game.name || "", icon: game.icon || "applications-games", game: game,
                       friends: root.friendsFor(game).length })
        }
        return out
    }
    onEntriesChanged: list.currentIndex = 0

    ListView {
        id: list
        anchors.fill: parent
        clip: true
        model: results.entries
        spacing: 2
        currentIndex: 0
        highlightMoveDuration: 0
        boundsBehavior: Flickable.StopAtBounds
        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}

        section.property: "section"
        section.delegate: RowLayout {
            required property string section
            width: ListView.view ? ListView.view.width : 0
            height: implicitHeight + Kirigami.Units.smallSpacing * 2
            spacing: Kirigami.Units.smallSpacing
            PlasmaComponents.Label {
                Layout.leftMargin: Kirigami.Units.smallSpacing
                Layout.topMargin: Kirigami.Units.smallSpacing
                text: section
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.6
                opacity: 0.65
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.smallSpacing
                implicitHeight: 1
                color: Qt.alpha(Kirigami.Theme.textColor, 0.1)
            }
        }

        delegate: Rectangle {
            id: row
            required property var modelData
            required property int index
            readonly property bool current: ListView.isCurrentItem
            readonly property bool isGame: modelData.kind === "game"
            readonly property string art: isGame ? (modelData.game.header || modelData.game.hero || "") : ""

            width: ListView.view.width - Kirigami.Units.smallSpacing
            height: Kirigami.Units.gridUnit * 2.6
            radius: Kirigami.Units.cornerRadius * 2
            color: current ? Qt.alpha(Kirigami.Theme.highlightColor, 0.2)
                 : rowHover.hovered ? Qt.alpha(Kirigami.Theme.highlightColor, 0.12) : "transparent"
            border.width: current ? 1 : 0
            border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.45)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Kirigami.Units.smallSpacing * 1.5
                anchors.rightMargin: Kirigami.Units.largeSpacing
                spacing: Kirigami.Units.largeSpacing

                Item {
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                    Layout.preferredWidth: row.art !== "" ? Math.round(Layout.preferredHeight * 460 / 215) : Layout.preferredHeight

                    Kirigami.Icon {
                        anchors.fill: parent
                        visible: row.art === ""
                        source: row.modelData.icon
                    }
                    Image {
                        id: artImage
                        anchors.fill: parent
                        visible: false
                        source: row.art !== "" ? "file://" + encodeURI(row.art) : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                    }
                    Rectangle {
                        id: artMask
                        anchors.fill: parent
                        radius: Kirigami.Units.cornerRadius
                        visible: false
                    }
                    OpacityMask {
                        anchors.fill: parent
                        visible: row.art !== ""
                        source: artImage
                        maskSource: artMask
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    PlasmaComponents.Label {
                        Layout.fillWidth: true
                        text: Highlight.mark(row.modelData.name, results.query.trim(), Kirigami.Theme.highlightColor)
                        textFormat: Text.StyledText
                        elide: Text.ElideRight
                        font.weight: Font.DemiBold
                    }
                    PlasmaComponents.Label {
                        Layout.fillWidth: true
                        text: row.modelData.kind === "fav" ? i18n("Favourite")
                            : row.isGame ? (row.modelData.friends > 0 ? i18np("Game · %1 friend playing", "Game · %1 friends playing", row.modelData.friends) : i18n("Game"))
                            : i18n("Application")
                        color: row.isGame && row.modelData.friends > 0 ? "#43a047" : Kirigami.Theme.textColor
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.65
                        elide: Text.ElideRight
                    }
                }

                PlasmaComponents.Label {
                    visible: row.current
                    text: i18n("Enter ↵")
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    opacity: 0.5
                }
            }

            HoverHandler { id: rowHover; cursorShape: Qt.PointingHandCursor }
            TapHandler {
                onTapped: results.activate(row.modelData)
            }
        }
    }

    PlasmaExtras.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: results.needle !== "" && list.count === 0
        iconName: "edit-find"
        text: i18n("Nothing matches “%1”", results.query.trim())
        explanation: i18n("Search covers favourites, every application and your games.")
    }
}
