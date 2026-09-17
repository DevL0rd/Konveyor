import QtQuick
import org.kde.konveyor.components

MiniScreen {
    id: frame

    property real aspect: 16 / 9
    default property alias screenContent: screen.data
    readonly property alias screen: screen

    width: height * aspect
    clip: false

    Item {
        id: screen
        anchors.fill: parent
        anchors.margins: 5
        clip: true
    }
}
