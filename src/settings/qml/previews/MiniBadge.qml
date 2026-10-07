import QtQuick
import org.kde.kirigami as Kirigami

Rectangle {
    id: badge

    property string label
    property real size: 12

    width: Math.max(size, text.implicitWidth + size * 0.4)
    height: size
    radius: size / 2
    color: Kirigami.Theme.highlightColor
    opacity: label.length > 0 ? 1 : 0
    scale: label.length > 0 ? 1 : 0.6

    Behavior on opacity {
        NumberAnimation { duration: Kirigami.Units.shortDuration }
    }
    Behavior on scale {
        NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.OutBack }
    }

    Text {
        id: text
        anchors.centerIn: parent
        text: badge.label
        color: Kirigami.Theme.highlightedTextColor
        font.pixelSize: badge.size * 0.7
        font.weight: Font.DemiBold
    }
}
