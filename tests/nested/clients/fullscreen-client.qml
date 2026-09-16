import QtQuick
import QtQuick.Window

Window {
    id: root

    visible: true
    width: 600
    height: 400
    title: "SelfFullscreen"
    color: "#b03030"

    Timer {
        interval: 1500
        running: true
        onTriggered: root.visibility = Window.FullScreen
    }

    Timer {
        interval: 12000
        running: true
        onTriggered: root.visibility = Window.Windowed
    }
}
