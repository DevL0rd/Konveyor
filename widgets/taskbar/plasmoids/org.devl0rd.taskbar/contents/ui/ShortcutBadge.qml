import QtQuick
import org.kde.kirigami as Kirigami

Rectangle {
    id: badge

    property string label
    property bool shown: false

    readonly property int size: Math.round(Kirigami.Units.gridUnit * 0.9)

    visible: opacity > 0
    opacity: shown && label.length > 0 ? 1 : 0
    scale: shown ? 1 : 0.6
    width: Math.max(size, text.implicitWidth + Kirigami.Units.smallSpacing * 2)
    height: size
    radius: height / 2
    color: Kirigami.Theme.highlightColor
    border.width: 1
    border.color: Qt.rgba(Kirigami.Theme.backgroundColor.r, Kirigami.Theme.backgroundColor.g, Kirigami.Theme.backgroundColor.b, 0.7)
    z: 10

    Behavior on opacity {
        NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.OutCubic }
    }
    Behavior on scale {
        NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.OutBack }
    }

    Text {
        id: text
        anchors.centerIn: parent
        text: badge.label
        color: Kirigami.Theme.highlightedTextColor
        font.pixelSize: Math.round(badge.size * 0.68)
        font.weight: Font.DemiBold
    }
}
