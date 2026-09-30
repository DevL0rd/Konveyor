import QtQuick
import QtQuick.Window
import Qt.labs.folderlistmodel

Window {
    id: root

    readonly property var args: Qt.application.arguments

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

    FolderListModel {
        folder: "file://" + root.args[root.args.length - 1]
        nameFilters: ["leave-fullscreen"]
        showDirs: false
        onCountChanged: {
            if (count > 0) {
                root.visibility = Window.Windowed
            }
        }
    }
}
