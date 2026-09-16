import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import Qt5Compat.GraphicalEffects
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras

Item {
    id: gv
    property string viewMode: "grid"
    property int cardWidth: 150
    property string searchText: ""
    property string sortMode: "recent"
    property var usage: ({})
    property bool showTitles: true
    readonly property var games: root.games
    readonly property bool loading: root.gamesLoading
    readonly property var friendsByAppid: root.friendsByAppid
    property bool friendsOnly: false

    readonly property bool isCarousel: viewMode === "carousel" || viewMode === "carousel3d"
    function browse(step) {
        var v = viewMode === "carousel" ? cfFlat : (viewMode === "carousel3d" ? cf3d : null)
        if (v) v.browse(step)
    }

    function fileUrl(p) { return p ? "file://" + encodeURI(p) : "" }
    function friendsFor(g) { return root.friendsFor(g) }
    function launch(g) { root.launchGame(g) }
    function steamRun(url) { root.steamRun(url) }
    function openStore(g) { if (g && g.appid) root.run("steam steam://store/" + g.appid) }
    function resetArt(g) { root.resetArt(g) }
    function setArt(g, url) { root.setArt(g, url) }
    function reload() { root.reloadGames() }
    function activateFirst() { if (view.length > 0) launch(view[0]) }
    function lastOf(g) { return Math.max(gv.usage[g.id] || 0, g.last || 0) }

    readonly property var view: {
        var q = gv.searchText.trim().toLowerCase()
        var a = gv.games.filter(function(g) {
            if (q !== "" && (g.name || "").toLowerCase().indexOf(q) < 0) return false
            if (gv.friendsOnly && gv.friendsFor(g).length === 0) return false
            return true
        })
        a = a.slice()
        if (gv.sortMode === "name") a.sort(function(x, y) { return x.name.localeCompare(y.name) })
        else if (gv.sortMode === "name_desc") a.sort(function(x, y) { return y.name.localeCompare(x.name) })
        else a.sort(function(x, y) {
            var d = gv.lastOf(y) - gv.lastOf(x)
            return d !== 0 ? d : x.name.localeCompare(y.name)
        })
        return a
    }

    property var _pendingArtGame: null
    FileDialog {
        id: artDialog
        title: i18n("Choose game art")
        nameFilters: [i18n("Images (*.png *.jpg *.jpeg *.webp)")]
        onAccepted: if (gv._pendingArtGame) gv.setArt(gv._pendingArtGame, selectedFile)
    }
    function pickArt(g) { gv._pendingArtGame = g; artDialog.open() }

    QQC2.Menu {
        id: gameMenu
        property var game: null
        property var friends: []
        QQC2.MenuItem { text: i18n("Launch"); icon.name: "media-playback-start"; onTriggered: gv.launch(gameMenu.game) }
        QQC2.MenuSeparator {}
        QQC2.MenuItem { text: i18n("Set custom art…"); icon.name: "insert-image"; onTriggered: gv.pickArt(gameMenu.game) }
        QQC2.MenuItem { text: i18n("Reset art"); icon.name: "edit-undo"; onTriggered: gv.resetArt(gameMenu.game) }
        QQC2.MenuItem {
            text: i18n("View on Steam"); icon.name: "steam"
            enabled: gameMenu.game && gameMenu.game.appid
            onTriggered: gv.openStore(gameMenu.game)
        }
        QQC2.MenuSeparator {
            visible: gameMenu.friends.length > 0
            height: visible ? implicitHeight : 0
        }
        QQC2.MenuItem {
            enabled: false
            visible: gameMenu.friends.length > 0
            height: visible ? implicitHeight : 0
            icon.name: "im-user"
            text: i18np("%1 friend playing", "%1 friends playing", gameMenu.friends.length)
        }
        Instantiator {
            model: gameMenu.friends
            delegate: QQC2.Menu {
                required property var modelData
                title: modelData.name
                QQC2.MenuItem { text: i18n("Join Game"); icon.name: "media-playback-start"; onTriggered: gv.steamRun(modelData.join) }
                QQC2.MenuItem { text: i18n("Profile"); icon.name: "user-identity"; onTriggered: gv.steamRun(modelData.profile) }
                QQC2.MenuItem { text: i18n("Watch Game"); icon.name: "video-television"; onTriggered: gv.steamRun(modelData.watch) }
                QQC2.MenuItem { text: i18n("Send Message"); icon.name: "mail-message"; onTriggered: gv.steamRun(modelData.message) }
            }
            onObjectAdded: function(index, object) { gameMenu.addMenu(object) }
            onObjectRemoved: function(index, object) { gameMenu.removeMenu(object) }
        }
    }
    function popMenu(g) { gameMenu.game = g; gameMenu.friends = gv.friendsFor(g); gameMenu.popup() }

    component BannerTile: Rectangle {
        id: tile
        property var game: ({})
        radius: Kirigami.Units.smallSpacing
        clip: true
        color: Qt.darker(Kirigami.Theme.backgroundColor, 1.3)
        Image {
            anchors.fill: parent
            visible: tile.game && (tile.game.hero || tile.game.header)
            source: tile.game ? gv.fileUrl(tile.game.hero || tile.game.header) : ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true; cache: true
        }
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.8) }
                GradientStop { position: 0.6; color: Qt.rgba(0, 0, 0, 0.2) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
        Image {
            anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Kirigami.Units.largeSpacing
            height: tile.height * 0.55; width: tile.width * 0.42
            visible: tile.game && tile.game.logo
            source: tile.game && tile.game.logo ? gv.fileUrl(tile.game.logo) : ""
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignLeft
            asynchronous: true; cache: true
        }
        Kirigami.Icon {
            anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Kirigami.Units.largeSpacing
            width: tile.height * 0.5; height: width
            visible: tile.game && !tile.game.logo && !tile.game.hero && !tile.game.header
            source: tile.game ? (tile.game.icon || "applications-games") : ""
        }
        PlasmaComponents.Label {
            anchors.left: parent.left; anchors.bottom: parent.bottom
            anchors.margins: Kirigami.Units.largeSpacing
            visible: tile.game && !tile.game.logo
            text: tile.game ? (tile.game.name || "") : ""
            color: "white"; font.weight: Font.Bold
        }
        Rectangle {
            id: tileBadge
            anchors.top: parent.top; anchors.right: parent.right
            anchors.margins: Kirigami.Units.smallSpacing
            readonly property int n: gv.friendsFor(tile.game).length
            visible: n > 0
            z: 20
            height: Math.round(Kirigami.Units.gridUnit * 1.1)
            width: tileBadgeRow.implicitWidth + Kirigami.Units.smallSpacing * 1.5
            radius: height / 2
            color: "#43a047"
            border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.35)
            Row {
                id: tileBadgeRow
                anchors.centerIn: parent; spacing: 1
                Kirigami.Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.round(tileBadge.height * 0.7); height: width
                    source: "im-user"; color: "white"
                }
                PlasmaComponents.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: tileBadge.n
                    color: "white"; font.weight: Font.Bold
                    font.pixelSize: Math.round(tileBadge.height * 0.6)
                }
            }
        }
        signal playRequested()
        property bool armed: false
        property bool hovered: tileHover.hovered
        onHoveredChanged: if (!hovered) armed = false
        HoverHandler { id: tileHover }
        TapHandler { acceptedButtons: Qt.LeftButton; onTapped: tile.armed = true }
        Rectangle {
            anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: Kirigami.Units.largeSpacing
            visible: tile.armed
            width: tilePlayRow.implicitWidth + Kirigami.Units.largeSpacing * 4
            height: tilePlayRow.implicitHeight + Kirigami.Units.largeSpacing * 1.6
            radius: Kirigami.Units.smallSpacing
            color: tilePlay.pressed ? "#2e7d32" : "#43a047"
            scale: tilePlay.pressed ? 0.95 : 1.0
            Behavior on scale { NumberAnimation { duration: 80 } }
            Row {
                id: tilePlayRow
                anchors.centerIn: parent
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Kirigami.Units.iconSizes.smallMedium; height: width
                    source: "media-playback-start"; color: "white"
                }
                PlasmaComponents.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: i18n("Play"); color: "white"; font.weight: Font.Bold
                }
            }
            TapHandler { id: tilePlay; onTapped: tile.playRequested() }
        }
    }

    GridView {
        id: gridView
        visible: gv.viewMode === "grid"
        anchors.top: parent.top; anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.max(cellWidth, Math.floor(parent.width / cellWidth) * cellWidth)
        clip: true
        model: gv.viewMode === "grid" ? gv.view : []
        cellWidth: gv.cardWidth + Kirigami.Units.largeSpacing
        cellHeight: gv.cardWidth * 1.5 + Kirigami.Units.largeSpacing
        boundsBehavior: Flickable.StopAtBounds
        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        delegate: Item {
            width: gridView.cellWidth
            height: gridView.cellHeight
            GameCard {
                id: gridCard
                anchors.centerIn: parent
                width: gv.cardWidth; height: gv.cardWidth * 1.5
                game: modelData; showTitle: gv.showTitles
                friendCount: gv.friendsFor(modelData).length
                onCardClicked: gridCard.armed = true
                onLaunchRequested: gv.launch(modelData)
                onMenuRequested: gv.popMenu(modelData)
            }
        }
    }

    ListView {
        id: listView
        visible: gv.viewMode === "list"
        anchors.fill: parent
        clip: true
        model: gv.viewMode === "list" ? gv.view : []
        spacing: Kirigami.Units.smallSpacing
        boundsBehavior: Flickable.StopAtBounds
        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        delegate: Item {
            width: listView.width
            height: Kirigami.Units.gridUnit * 3.5
            RectangularGlow {
                anchors.fill: rowTile
                visible: gv.friendsFor(modelData).length > 0
                z: -1
                glowRadius: Kirigami.Units.largeSpacing
                spread: 0.2
                color: "#43a047"
                cornerRadius: Kirigami.Units.smallSpacing + glowRadius
            }
            BannerTile {
                id: rowTile
                anchors.fill: parent
                game: modelData
                scale: rowHover.hovered ? 1.01 : 1.0
                Behavior on scale { NumberAnimation { duration: 110 } }
                onPlayRequested: gv.launch(modelData)
            }
            HoverHandler { id: rowHover }
            TapHandler { acceptedButtons: Qt.RightButton; onTapped: gv.popMenu(modelData) }
        }
    }

    CoverFlow {
        id: cfFlat
        anchors.fill: parent
        visible: gv.viewMode === "carousel"
        items: gv.viewMode === "carousel" ? gv.view : []
        friendsByAppid: gv.friendsByAppid
        tilt: false
        shrink: false
        cardWidth: gv.cardWidth
        showTitles: gv.showTitles
        onLaunchRequested: function(g) { gv.launch(g) }
        onMenuRequested: function(g) { gv.popMenu(g) }
    }

    CoverFlow {
        id: cf3d
        anchors.fill: parent
        visible: gv.viewMode === "carousel3d"
        items: gv.viewMode === "carousel3d" ? gv.view : []
        friendsByAppid: gv.friendsByAppid
        tilt: true
        cardWidth: gv.cardWidth
        showTitles: gv.showTitles
        onLaunchRequested: function(g) { gv.launch(g) }
        onMenuRequested: function(g) { gv.popMenu(g) }
    }

    GridView {
        id: bannerView
        visible: gv.viewMode === "banner"
        anchors.fill: parent
        clip: true
        model: gv.viewMode === "banner" ? gv.view : []
        property int bw: gv.cardWidth * 2
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / bw)))
        cellHeight: cellWidth * 0.34
        boundsBehavior: Flickable.StopAtBounds
        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        delegate: Item {
            width: bannerView.cellWidth
            height: bannerView.cellHeight
            RectangularGlow {
                anchors.fill: bTile
                visible: gv.friendsFor(modelData).length > 0
                z: -1
                glowRadius: Kirigami.Units.smallSpacing * 2
                spread: 0.2
                color: "#43a047"
                cornerRadius: Kirigami.Units.smallSpacing + glowRadius
            }
            BannerTile {
                id: bTile
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing / 2
                game: modelData
                scale: bHover.hovered ? 1.02 : 1.0
                Behavior on scale { NumberAnimation { duration: 110 } }
                onPlayRequested: gv.launch(modelData)
            }
            HoverHandler { id: bHover }
            TapHandler { acceptedButtons: Qt.RightButton; onTapped: gv.popMenu(modelData) }
        }
    }

    PlasmaExtras.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: gv.loading && gv.games.length === 0
        iconName: "applications-games"
        text: i18n("Loading games…")
    }
    PlasmaExtras.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !gv.loading && gv.view.length === 0
        iconName: gv.searchText !== "" ? "edit-find" : "applications-games"
        text: gv.searchText !== "" ? i18n("No games match")
            : gv.friendsOnly ? i18n("No games with friends online")
            : i18n("No games found")
        explanation: gv.friendsOnly && gv.searchText === "" ? i18n("Turn off \"Friends online only\" in the sort menu to see everything.") : ""
    }
}
