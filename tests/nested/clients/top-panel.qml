import QtQuick
import QtQuick.Window
import org.kde.layershell as LayerShell

Window {
    height: 46
    visible: true
    color: "#202124"

    LayerShell.Window.scope: "dock"
    LayerShell.Window.layer: LayerShell.Window.LayerTop
    LayerShell.Window.anchors: LayerShell.Window.AnchorTop | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
    LayerShell.Window.exclusionZone: 46
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
}
