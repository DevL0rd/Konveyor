import QtQuick
import org.kde.plasma.plasma5support as P5Support

Item {
    id: watchdog

    property bool active: false
    property int staleAfter: 4000
    property string restartCommand: ""
    property bool stale: false
    property bool restarted: false

    function beat() {
        stale = false
        restarted = false
        if (active)
            deadline.restart()
    }

    onActiveChanged: active ? deadline.restart() : deadline.stop()
    Component.onCompleted: if (active) deadline.restart()

    Timer {
        id: deadline
        interval: watchdog.staleAfter
        onTriggered: {
            watchdog.stale = true
            if (watchdog.restarted || watchdog.restartCommand === "")
                return
            watchdog.restarted = true
            runner.connectSource(watchdog.restartCommand)
        }
    }

    P5Support.DataSource {
        id: runner
        engine: "executable"
        onNewData: function(source, data) { disconnectSource(source) }
    }
}
