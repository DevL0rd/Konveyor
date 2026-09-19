import QtQuick
import QtQuick.Window

Window {
    id: root

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
            const until = Date.now() + 2000
            while (Date.now() < until) {
            }
        }
    }
}
