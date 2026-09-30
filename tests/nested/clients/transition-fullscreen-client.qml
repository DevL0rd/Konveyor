import QtQuick
import QtQuick.Window

Window {
    id: root

    readonly property var args: Qt.application.arguments

    function exists(path) {
        const request = new XMLHttpRequest()
        request.open("GET", "file://" + path, false)
        request.send()
        return request.responseText.length > 0
    }

    visible: true
    width: 600
    height: 400
    title: "TransitionWindowed"
    color: "#b03030"

    Timer {
        interval: 1000
        running: true
        onTriggered: {
            root.title = "TransitionPending"
            root.visibility = Window.FullScreen
            block.start()
        }
    }

    Timer {
        id: block

        interval: 0
        onTriggered: {
            while (!root.exists(root.args[root.args.length - 1])) {
            }
        }
    }
}
