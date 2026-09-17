import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Rectangle {
    id: window

    property real lift: 0
    property bool alternate: false
    property real titleHeight: height * 0.14
    property int tabs: 0
    property int tab: 0
    property real tabbed: 0
    property string label
    property bool compact: false

    radius: compact ? 1.5 : 3
    color: Kirigami.Theme.backgroundColor
    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.25)
    border.width: 1

    Rectangle {
        width: parent.width
        height: Math.min(window.titleHeight, parent.height * 0.4)
        radius: window.radius
        color: window.alternate ? Qt.alpha(Kirigami.Theme.positiveTextColor, 0.3) : Qt.alpha(Kirigami.Theme.textColor, 0.1)

        Row {
            visible: window.tabs > 0 && window.tabbed > 0
            opacity: window.tabbed
            anchors.fill: parent
            anchors.margins: Math.max(1, parent.height * 0.2)
            spacing: Math.max(1, parent.height * 0.15)

            Repeater {
                model: window.tabs

                Rectangle {
                    required property int index
                    width: (parent.width - parent.spacing * (window.tabs - 1)) / window.tabs
                    height: parent.height
                    radius: height / 2
                    color: index === window.tab ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.3)
                }
            }
        }
    }

    QQC2.Label {
        visible: window.label !== "" && !window.compact
        anchors.centerIn: parent
        anchors.verticalCenterOffset: window.titleHeight / 2
        text: window.label
        font.pixelSize: Math.max(8, Math.min(parent.height * 0.18, parent.width * 0.22))
        font.weight: Font.DemiBold
        color: window.lift > 0.5 ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
        opacity: 0.85
    }

    Rectangle {
        anchors.fill: parent
        radius: window.radius
        color: "transparent"
        border.color: Kirigami.Theme.highlightColor
        border.width: window.compact ? 1.5 : 2
        opacity: window.lift
        visible: opacity > 0.01
    }
}
