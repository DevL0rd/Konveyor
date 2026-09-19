import QtQuick
import QtQuick.Window

Window {
    readonly property var args: Qt.application.arguments

    width: Number(args[args.length - 2])
    height: Number(args[args.length - 1])
    visible: true
    title: args[args.length - 3]
    flags: Qt.FramelessWindowHint
    color: "#252525"
}
