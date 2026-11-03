import QtQuick
import QtQuick.Window

Window {
    id: window

    readonly property var args: Qt.application.arguments

    width: 200
    height: 32
    visible: true
    title: args[args.length - 2]
    flags: Qt.FramelessWindowHint
    color: "#252525"

    Timer {
        interval: 2000
        running: true
        onTriggered: window.title = window.args[window.args.length - 1]
    }
}
