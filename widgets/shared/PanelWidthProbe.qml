import QtQuick

Timer {
    id: probe

    property real tileSpan: 0
    property real fullSpan: 0
    property real screenSpan: 0
    property bool probing: true
    property real settled: 0
    readonly property real settledSpan: !probing && settled > 0 ? settled : 0

    function reprobe() {
        probing = true
        settled = 0
        restart()
    }

    interval: 200
    running: true
    onFullSpanChanged: reprobe()
    onScreenSpanChanged: reprobe()
    onTriggered: {
        settled = tileSpan
        probing = false
    }
}
