import QtQuick
import "../catalog/Kdl.js" as Kdl
import org.kde.kquickcontrols as KQuickControls

KQuickControls.ColorButton {
    id: root

    property string value
    signal edited(string css)

    function sync() {
        color = value.length > 0 ? value : "transparent";
    }

    onValueChanged: sync()
    Component.onCompleted: sync()
    showAlphaChannel: true
    onAccepted: picked => root.edited(Kdl.cssColor(picked))
}
