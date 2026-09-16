import QtQuick
import QtQuick.Window

Window {
    id: root

    readonly property var args: Qt.application.arguments
    readonly property string label: args[args.length - 3]

    width: Number(args[args.length - 2])
    height: Number(args[args.length - 1])
    visible: true
    title: root.label
    color: "#2f3033"

    Text {
        anchors.centerIn: parent
        text: root.label
        color: "#f0f0f0"
        font.pixelSize: 40
    }
}
