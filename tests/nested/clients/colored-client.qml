import QtQuick
import QtQuick.Window

Window {
    id: root

    readonly property var args: Qt.application.arguments
    readonly property string label: args[args.length - 3]
    readonly property var colors: ({"A": "#c80000", "B": "#00c800", "C": "#0000c8", "D": "#c8c800"})

    width: Number(args[args.length - 2])
    height: Number(args[args.length - 1])
    visible: true
    title: root.label
    color: root.colors[root.label] || "#2f3033"

    MouseArea {
        anchors.fill: parent
        onClicked: console.warn("konveyor-test-clicked:" + root.label)
    }
}
