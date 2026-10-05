import QtQuick

Item {
    anchors.fill: parent
    z: 1000
    PointHandler {
        acceptedDevices: PointerDevice.TouchScreen
        onActiveChanged: {
            launcher.touchDown = active
            if (active)
                launcher.touchMode = true
            else if (launcher.pendingMenu)
                Qt.callLater(launcher.showPendingMenu)
        }
    }
    PointHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onActiveChanged: if (active) launcher.touchMode = false
    }
}
