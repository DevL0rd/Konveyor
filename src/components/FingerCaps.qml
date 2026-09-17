import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Rectangle {
    id: caps

    property int fingers: 3
    property string motion: "swipe-horizontal"
    property string device: "touchpad"

    readonly property string symbol: {
        switch (motion) {
        case "swipe-horizontal":
            return "←→";
        case "swipe-vertical":
            return "↑↓";
        case "pinch":
            return "⤡";
        case "window-swipe-horizontal":
            return "▭ ←→";
        case "window-swipe-vertical":
            return "▭ ↑↓";
        case "long-press":
            return "Hold";
        }
        return "";
    }
    readonly property real dot: Kirigami.Units.gridUnit * 0.42

    implicitWidth: row.implicitWidth + Kirigami.Units.largeSpacing
    implicitHeight: Math.max(row.implicitHeight, symbolLabel.implicitHeight) + Kirigami.Units.smallSpacing * 2
    radius: Kirigami.Units.cornerRadius
    color: Qt.alpha(Kirigami.Theme.highlightColor, 0.12)
    border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.6)

    Accessible.name: caps.fingers + (caps.fingers === 1 ? " finger " : " fingers ") + caps.motion.replace(/-/g, " ") + " on the " + caps.device

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: Kirigami.Units.smallSpacing

        Item {
            implicitWidth: caps.fingers * caps.dot + (caps.fingers - 1) * caps.dot * 0.35
            implicitHeight: caps.dot * 1.5

            Repeater {
                model: caps.fingers

                Rectangle {
                    required property int index
                    readonly property real middle: (caps.fingers - 1) / 2
                    x: index * caps.dot * 1.35
                    y: Math.abs(index - middle) * caps.dot * 0.25
                    width: caps.dot
                    height: caps.dot
                    radius: width / 2
                    color: Kirigami.Theme.highlightColor
                }
            }
        }

        QQC2.Label {
            id: symbolLabel
            text: caps.symbol
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            font.weight: Font.DemiBold
        }
    }
}
