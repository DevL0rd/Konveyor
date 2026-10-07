import QtQuick
import org.kde.kirigami as Kirigami

Rectangle {
    id: pill

    property real size: 32
    property bool current: false
    property int mode: 0
    property int number: 1
    property int columns: 0
    property string badge

    readonly property real dot: Math.max(2, size * 0.1)
    readonly property color ink: current ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor

    width: Math.max(height, (mode === 0 ? dots.width : label.implicitWidth) + height * 0.6)
    height: size * 0.46
    radius: height / 2
    color: current ? Kirigami.Theme.highlightColor : "transparent"

    Behavior on width {
        NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
    }

    Row {
        id: dots
        visible: pill.mode === 0
        anchors.centerIn: parent
        spacing: pill.dot * 0.6

        Repeater {
            model: Math.max(1, pill.columns)

            Rectangle {
                required property int index
                width: pill.columns > 0 && index === 1 ? pill.dot * 2.2 : pill.dot
                height: pill.dot
                radius: pill.dot / 2
                color: pill.columns > 0 ? pill.ink : "transparent"
                border.width: pill.columns > 0 ? 0 : 1
                border.color: pill.ink
                opacity: index === 1 || pill.columns === 0 ? 1 : 0.7
            }
        }
    }

    Text {
        id: label
        visible: pill.mode !== 0
        anchors.centerIn: parent
        text: pill.mode === 1 && pill.number === 1 ? "web" : String(pill.number)
        color: pill.ink
        font.pixelSize: pill.height * 0.6
    }

    MiniBadge {
        label: pill.badge
        size: pill.size * 0.34
        x: pill.width - width * 0.6
        y: -height * 0.4
    }
}
