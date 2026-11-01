import QtQuick
import QtQuick.Window

Window {
    id: root

    readonly property var args: Qt.application.arguments
    readonly property string label: args[args.length - 7]
    readonly property bool modal: args[args.length - 4] === "modal"

    width: Number(args[args.length - 6])
    height: Number(args[args.length - 5])
    visible: true
    title: root.label
    color: "#2f3033"

    Timer {
        interval: Number(root.args[root.args.length - 3])
        running: true
        onTriggered: dialog.visible = true
    }

    Window {
        id: dialog

        width: Number(root.args[root.args.length - 2])
        height: Number(root.args[root.args.length - 1])
        title: root.label + "-Dialog"
        transientParent: root
        flags: Qt.Dialog
        modality: root.modal ? Qt.WindowModal : Qt.NonModal
        color: "#4a6fa5"
        onActiveChanged: console.warn("konveyor-test-active:" + title + ":" + active)

        MouseArea {
            anchors.fill: parent
            onClicked: console.warn("konveyor-test-clicked:" + dialog.title)
        }
    }
}
