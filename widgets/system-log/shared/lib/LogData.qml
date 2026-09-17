import QtQuick
import org.kde.plasma.plasma5support as P5Support

Item {
    id: root

    property int interval: 1000
    property bool paused: false
    property bool active: true

    property var lines: []
    property double ts: 0
    property bool online: false
    property bool ready: false
    signal updated()

    property string cachePath: ""

    P5Support.DataSource {
        id: helper
        engine: "executable"
        onNewData: function(source, d) {
            root.cachePath = (d.stdout || "").trim()
            root.read()
            disconnectSource(source)
        }
    }

    function read() {
        if (!root.cachePath || root.paused || !root.active)
            return
        var xhr = new XMLHttpRequest()
        xhr.open("GET", "file://" + root.cachePath)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            if (!xhr.responseText) {
                root.online = false
                return
            }
            try {
                var parsed = JSON.parse(xhr.responseText)
                root.ts = parsed.ts || 0
                root.online = parsed.alive !== false
                    && (Date.now() / 1000 - root.ts) < 15
                root.lines = parsed.lines || []
                root.ready = true
                root.updated()
            } catch (e) {}
        }
        xhr.send()
    }

    FileWatcher { path: root.active ? root.cachePath : ""; onChanged: root.read() }

    Component.onCompleted: helper.connectSource("printf %s \"$XDG_RUNTIME_DIR/Linux-Log-Monitor/log.json\"")
}
