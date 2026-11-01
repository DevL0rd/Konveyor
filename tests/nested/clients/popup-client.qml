import QtQuick
import QtQuick.Controls
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

    Menu {
        id: menu

        popupType: Popup.Window
        width: 1200

        MenuItem { text: "First" }
        MenuItem { text: "Second" }
        MenuItem { text: "Third" }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: mouse => { menu.x = mouse.x; menu.y = mouse.y; menu.open() }
    }
}
