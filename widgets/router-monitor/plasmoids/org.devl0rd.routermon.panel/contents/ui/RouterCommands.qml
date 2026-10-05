import QtQuick
import org.kde.plasma.plasmoid
import org.kde.plasma.plasma5support as P5Support
import "lib"

PlasmoidItem {
    id: root

    readonly property string ctl: "$HOME/.local/bin/routermon-ctl"

    property string message
    property bool messageError: false
    Timer {
        id: messageTimer
        interval: 5000
        onTriggered: root.message = ""
    }
    function flashMessage(text, isError) {
        message = text
        messageError = isError === true
        messageTimer.restart()
    }

    P5Support.DataSource {
        id: ctlRunner
        engine: "executable"
        onNewData: function(source, data) {
            try {
                const result = JSON.parse(data.stdout)
                root.flashMessage(result.msg || "", result.ok === false)
            } catch (error) {
                root.flashMessage((data.stderr || "").trim(), true)
            }
            disconnectSource(source)
        }
    }
    function shq(text) {
        return "'" + String(text).replace(/'/g, "'\\''") + "'"
    }
    function ctlRun(args) {
        ctlRunner.connectSource(ctl + " " + args)
    }

    P5Support.DataSource {
        id: localRunner
        engine: "executable"
        onNewData: function(source, data) { disconnectSource(source) }
    }
    function launch(command) {
        localRunner.connectSource(command)
    }

    TextEdit { id: clipboard; visible: false }
    function copy(text) {
        clipboard.text = text
        clipboard.selectAll()
        clipboard.copy()
        flashMessage(i18n("Copied %1", text), false)
    }

    readonly property var lastResult: {
        const text = Plasmoid.configuration.lastResult
        if (!text) return null
        try { return JSON.parse(text) } catch (error) { return null }
    }
    readonly property var speedHistory: {
        try { return JSON.parse(Plasmoid.configuration.speedHistory || "[]") } catch (error) { return [] }
    }
    property bool testing: false
    property var live: null
    P5Support.DataSource {
        id: speedRunner
        engine: "executable"
        onNewData: function(source, data) {
            const text = (data.stdout || "").trim()
            if (text) {
                try {
                    const result = JSON.parse(text)
                    Plasmoid.configuration.lastResult = text
                    if (result.ok !== false) {
                        if (result.down_mbps) Plasmoid.configuration.peakDown = result.down_mbps
                        if (result.up_mbps) Plasmoid.configuration.peakUp = result.up_mbps
                        const next = [result].concat(root.speedHistory).slice(0, 20)
                        Plasmoid.configuration.speedHistory = JSON.stringify(next)
                    } else {
                        root.flashMessage(result.error || i18n("Speed test failed"), true)
                    }
                } catch (error) {
                    root.flashMessage(i18n("Speed test failed"), true)
                }
            }
            root.testing = false
            root.live = null
            disconnectSource(source)
        }
    }
    FileWatcher {
        path: root.testing && routerData.cachePath ? routerData.cachePath.replace(/data\.json$/, "speedtest_live.json") : ""
        onChanged: {
            const xhr = new XMLHttpRequest()
            xhr.open("GET", "file://" + path)
            xhr.onreadystatechange = function() {
                if (xhr.readyState !== XMLHttpRequest.DONE || !xhr.responseText || !root.testing)
                    return
                try { root.live = JSON.parse(xhr.responseText) } catch (error) {}
            }
            xhr.send()
        }
    }
    function runSpeedTest() {
        if (testing)
            return
        live = null
        testing = true
        speedRunner.connectSource("$HOME/.local/bin/routermon-speedtest")
    }
    function deleteSpeedResult(index) {
        const next = speedHistory.slice()
        next.splice(index, 1)
        Plasmoid.configuration.speedHistory = JSON.stringify(next)
    }
    function ago(ts) {
        if (!ts) return ""
        const seconds = Math.max(0, Date.now() / 1000 - ts)
        if (seconds < 90) return i18n("just now")
        if (seconds < 3600) return i18np("%1 minute ago", "%1 minutes ago", Math.round(seconds / 60))
        if (seconds < 86400) return i18np("%1 hour ago", "%1 hours ago", Math.round(seconds / 3600))
        return i18np("%1 day ago", "%1 days ago", Math.round(seconds / 86400))
    }
}
