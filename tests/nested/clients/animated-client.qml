import QtQuick
import QtQuick.Window

Window {
    readonly property var args: Qt.application.arguments

    width: 400
    height: 300
    visible: true
    title: args[args.length - 1]
    color: "#2f3033"

    Rectangle {
        width: 80
        height: 80
        anchors.centerIn: parent
        color: "#1b91d5"

        RotationAnimation on rotation {
            from: 0
            to: 360
            duration: 1000
            loops: Animation.Infinite
        }
    }
}
