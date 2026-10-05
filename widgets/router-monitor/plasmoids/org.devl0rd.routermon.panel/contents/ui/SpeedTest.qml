import QtQuick
import org.kde.plasma.plasmoid
import org.kde.plasma.plasma5support as P5Support
import "lib"

Item {
    id: speedTest

    required property var router
    signal failed(string message)

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
                        const next = [result].concat(speedTest.speedHistory).slice(0, 20)
                        Plasmoid.configuration.speedHistory = JSON.stringify(next)
                    } else {
                        speedTest.failed(result.error || i18n("Speed test failed"))
                    }
                } catch (error) {
                    speedTest.failed(i18n("Speed test failed"))
                }
            }
            speedTest.testing = false
            speedTest.live = null
            disconnectSource(source)
        }
    }
    FileWatcher {
        path: speedTest.testing && speedTest.router.cachePath ? speedTest.router.cachePath.replace(/data\.json$/, "speedtest_live.json") : ""
        onChanged: {
            const xhr = new XMLHttpRequest()
            xhr.open("GET", "file://" + path)
            xhr.onreadystatechange = function() {
                if (xhr.readyState !== XMLHttpRequest.DONE || !xhr.responseText || !speedTest.testing)
                    return
                try { speedTest.live = JSON.parse(xhr.responseText) } catch (error) {}
            }
            xhr.send()
        }
    }
    function run() {
        if (testing)
            return
        live = null
        testing = true
        speedRunner.connectSource("$HOME/.local/bin/routermon-speedtest")
    }
    function deleteResult(index) {
        const next = speedHistory.slice()
        next.splice(index, 1)
        Plasmoid.configuration.speedHistory = JSON.stringify(next)
    }
}
