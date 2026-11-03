import QtQuick
import QtQuick.Window

Window {
    id: owner

    readonly property var args: Qt.application.arguments

    width: 400
    height: 300
    visible: true
    title: args[args.length - 3]
    color: "#2f3033"

    Window {
        id: card

        width: 200
        height: 32
        visible: true
        title: "Card"
        transientParent: owner
        flags: Qt.FramelessWindowHint | Qt.Dialog
        color: "#252525"
    }

    Timer {
        interval: 2000
        running: true
        onTriggered: card.title = owner.args[owner.args.length - 2]
    }

    Timer {
        interval: 4000
        running: true
        onTriggered: card.title = owner.args[owner.args.length - 1]
    }
}
