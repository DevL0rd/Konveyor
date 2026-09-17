import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import "lib"

PlasmoidItem {
    id: root

    readonly property string panelIcon: Plasmoid.configuration.icon || "view-app-grid-symbolic"
    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool open: inPanel ? expanded : visible
    readonly property bool openedByKey: false
    readonly property string openScreen: ""
    property var pendingPins: []
    signal pinsRequested()

    Plasmoid.icon: panelIcon
    Plasmoid.title: i18n("App Portal")

    function hide() {
        if (inPanel)
            expanded = false
    }

    property bool popupAlive: !inPanel
    preferredRepresentation: inPanel ? compactRepresentation : fullRepresentation
    onExpandedChanged: function() {
        if (expanded) {
            releasePopup.stop()
            popupAlive = true
        } else if (inPanel) {
            releasePopup.restart()
        }
    }
    Timer {
        id: releasePopup
        interval: 1500
        onTriggered: root.popupAlive = root.expanded || !root.inPanel
    }

    function migrate() {
        const config = Plasmoid.configuration
        if (config.migratedFromPortal)
            return
        const category = config.defaultCategory
        if (category === "Games")
            config.defaultPage = "games"
        else if (category === "Favorites" || category === "")
            config.defaultPage = "home"
        else {
            config.defaultPage = "apps"
            config.appsCategory = category
        }
        const views = { grid: "grid", list: "list", banner: "banner", carousel: "carousel", carousel3d: "coverflow" }
        if (views[config.gamesViewMode])
            config.gamesView = views[config.gamesViewMode]
        if (config.gameCardWidth > 0)
            config.gameCardSize = Math.max(6, Math.min(18, Math.round(config.gameCardWidth / Kirigami.Units.gridUnit)))
        if (config.appViewMode === "list")
            config.appsView = "list"
        if (config.defaultSort === "recent")
            config.gamesSort = "recent"
        else if (config.defaultSort === "name" || config.defaultSort === "name_desc")
            config.gamesSort = "name"
        config.migratedFromPortal = true
    }
    Component.onCompleted: migrate()

    property int friendsPlaying: 0
    property string friendsPath: ""
    property string friendsSignature: ""
    readonly property bool badgeWanted: inPanel && Plasmoid.configuration.showFriendsBadge && Plasmoid.configuration.showFriends
    P5Support.DataSource {
        id: pathHelper
        engine: "executable"
        connectedSources: root.badgeWanted && root.friendsPath === "" ? ["printf %s \"$XDG_RUNTIME_DIR/Plasma-App-Portal/friends.json\""] : []
        onNewData: function(source, data) {
            root.friendsPath = (data.stdout || "").trim()
            disconnectSource(source)
        }
    }
    function readFriends() {
        if (!friendsPath)
            return
        const xhr = new XMLHttpRequest()
        xhr.open("GET", "file://" + friendsPath)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            let byAppid = {}
            try {
                byAppid = JSON.parse(xhr.responseText || "{}").by_appid || {}
            } catch (error) {
                return
            }
            const signature = JSON.stringify(byAppid)
            if (signature === root.friendsSignature)
                return
            root.friendsSignature = signature
            let playing = 0
            for (const appid in byAppid)
                playing += byAppid[appid].length
            root.friendsPlaying = playing
        }
        xhr.send()
    }
    FileWatcher {
        path: root.badgeWanted ? root.friendsPath : ""
        onChanged: root.readFriends()
    }

    toolTipMainText: i18n("App Portal")
    toolTipSubText: friendsPlaying > 0 ? i18np("%1 friend in game", "%1 friends in game", friendsPlaying) : i18n("Apps, games, files and friends")

    compactRepresentation: CompactView {}
    fullRepresentation: PortalView {}
}
