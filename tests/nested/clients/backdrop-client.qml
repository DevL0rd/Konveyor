import QtQuick
import QtQuick.Window
import org.kde.layershell as LayerShell

Window {
    id: wallpaper

    visible: true
    title: "Wallpaper"
    color: "#00ff00"

    LayerShell.Window.scope: "desktop"
    LayerShell.Window.layer: LayerShell.Window.LayerBackground
    LayerShell.Window.anchors: LayerShell.Window.AnchorTop | LayerShell.Window.AnchorBottom | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
    LayerShell.Window.exclusionZone: -1
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone

    Window {
        transientParent: null
        height: 60
        visible: true
        title: "Taskbar"
        color: "#ffffff"

        LayerShell.Window.scope: "dock"
        LayerShell.Window.layer: LayerShell.Window.LayerTop
        LayerShell.Window.anchors: LayerShell.Window.AnchorBottom | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
        LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
    }
}
