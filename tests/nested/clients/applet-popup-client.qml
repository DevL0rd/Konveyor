import QtQuick
import org.kde.plasma.core as PlasmaCore

PlasmaCore.Dialog {
    id: dialog

    readonly property var args: Qt.application.arguments

    type: PlasmaCore.Dialog.AppletPopup
    location: PlasmaCore.Types.TopEdge
    x: Number(args[args.length - 2])
    y: Number(args[args.length - 1])
    visible: true
    hideOnWindowDeactivate: true
    mainItem: Rectangle {
        width: 400
        height: 300
        color: "#3f4043"
    }
}
