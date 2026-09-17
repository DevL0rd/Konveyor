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

    TextField {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20
        placeholderText: root.label
        onTextChanged: console.warn("konveyor-test-typed:" + root.label + ":" + text)
    }
}
