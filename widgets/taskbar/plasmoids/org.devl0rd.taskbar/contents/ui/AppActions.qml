import QtQuick
import org.kde.plasma.private.kicker as Kicker
import org.kde.plasma.private.mpris as Mpris

Item {
    id: actions

    property string appKey
    property int pid: 0

    readonly property string desktopId: appKey.replace(/^applications:/, "")
    readonly property var appActions: entries.count > 0 && entries.objectAt(0) ? entries.objectAt(0).actions : []

    function findPlayer() {
        const name = desktopId.replace(/\.desktop$/, "")
        for (let i = 0; i < players.count; ++i) {
            const each = players.objectAt(i)
            if (each && each.container && !each.multiplexer && (each.pid === pid && pid > 0 || each.entry === name))
                return each.container
        }
        return null
    }

    function playerState() {
        const found = findPlayer()
        return found ? { playing: found.playbackStatus === Mpris.PlaybackStatus.Playing, canGoNext: found.canGoNext,
            canGoPrevious: found.canGoPrevious, canControl: found.canControl, track: found.track || "" } : null
    }

    function trigger(action) {
        favorites.trigger(0, action.actionId, action.actionArgument)
    }

    function media(command) {
        const found = findPlayer()
        if (found)
            found[command]()
    }

    Kicker.SimpleFavoritesModel {
        id: favorites
        favorites: actions.desktopId ? [actions.desktopId] : []
    }

    Instantiator {
        id: entries
        model: favorites
        delegate: QtObject {
            required property var model
            readonly property var actions: model.actionList || []
        }
    }

    Mpris.Mpris2Model {
        id: mpris
    }

    Instantiator {
        id: players
        model: mpris
        delegate: QtObject {
            required property var model
            readonly property var container: model.container
            readonly property bool multiplexer: !!model.isMultiplexer
            readonly property int pid: model.instancePid || 0
            readonly property string entry: model.desktopEntry || ""
        }
    }
}
