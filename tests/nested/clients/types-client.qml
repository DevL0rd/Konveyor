import QtQuick
import QtQuick.Window
import org.kde.layershell as LayerShell

Window {
    id: main

    width: 600
    height: 400
    visible: true
    title: "W-Main"
    color: "#2f3033"

    Window {
        transientParent: main
        width: 300
        height: 200
        visible: true
        title: "W-Transient"
        color: "#3f4043"
    }

    Window {
        transientParent: main
        flags: Qt.Dialog
        modality: Qt.WindowModal
        width: 300
        height: 200
        visible: true
        title: "W-Modal"
        color: "#4f5053"
    }

    Window {
        transientParent: main
        flags: Qt.ToolTip
        width: 200
        height: 120
        visible: true
        title: "W-Popup"
        color: "#5f6063"
    }

    Window {
        LayerShell.Window.scope: "konveyor-test"
        LayerShell.Window.layer: LayerShell.Window.LayerTop
        LayerShell.Window.anchors: LayerShell.Window.AnchorTop
        width: 400
        height: 60
        visible: true
        title: "W-Layer"
        color: "#6f7073"
    }
}
