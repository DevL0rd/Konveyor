import QtQuick
import QtQuick.Window

Window {
    readonly property var args: Qt.application.arguments

    width: 500
    height: 400
    minimumWidth: 500
    maximumWidth: 500
    minimumHeight: 400
    maximumHeight: 400
    visible: true
    title: args[args.length - 1]
    color: "#2f3033"
}
