/*
 * Linux-Router-Monitor :: shared data source.
 *
 * Reads the snapshot kept in tmpfs by the resident `--serve` collector (the
 * systemd --user service), fully IN-PROCESS via XMLHttpRequest (file://) — no
 * process is spawned per poll. Requires QML_XHR_ALLOW_FILE_READ=1 in the Plasma
 * session (set by install.sh). No fallback: if the read can't happen (service
 * down / flag unset) the widget simply shows no data.
 */
import QtQuick
import org.kde.plasma.plasma5support as P5Support

Item {
    id: root

    property string section: "all"       // kept for API compat (whole file is read)
    property int interval: 2000
    property string tool: "$HOME/.local/bin/routermon-collect"

    property var snapshot: ({})
    property bool online: false
    property bool paused: false
    property bool ready: false
    property string error: ""
    signal updated()

    property string cachePath: ""
    property bool active: true

    // one-shot: resolve the runtime cache path (cheap shell echo), then poll via XHR
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
        if (!root.cachePath)
            return
        var xhr = new XMLHttpRequest()
        xhr.open("GET", "file://" + root.cachePath)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            if (!xhr.responseText)
                return                       // no data -> show nothing (no fallback)
            try {
                var parsed = JSON.parse(xhr.responseText)
                root.online = parsed.online !== false
                root.paused = parsed.paused === true
                root.error = parsed.error || ""
                if (!root.paused) {
                    root.snapshot = parsed
                    root.ready = true
                    root.updated()
                }
            } catch (e) {}
        }
        xhr.send()
    }

    // event-driven: re-read the instant the collector rewrites the snapshot (no polling)
    FileWatcher { path: root.active ? root.cachePath : ""; onChanged: root.read() }

    Component.onCompleted: helper.connectSource("printf %s \"$XDG_RUNTIME_DIR/Linux-Router-Monitor/data.json\"")
}
