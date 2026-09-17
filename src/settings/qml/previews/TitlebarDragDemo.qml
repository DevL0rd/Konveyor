import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.konveyor.components

Item {
    id: demo

    property string mode: "scroll-view"
    property bool selected: false
    property real progress: 0

    SequentialAnimation on progress {
        loops: Animation.Infinite
        running: demo.visible
        PauseAnimation { duration: 500 }
        NumberAnimation { from: 0; to: 1; duration: 1300; easing.type: Easing.InOutCubic }
        PauseAnimation { duration: 700 }
        NumberAnimation { from: 1; to: 0; duration: 900; easing.type: Easing.InOutCubic }
    }

    MiniScreen {
        id: frame
        readonly property real columnWidth: width * 0.3
        readonly property real gap: width * 0.04
        readonly property real step: columnWidth + gap
        readonly property real rowShift: demo.mode === "scroll-view" ? -demo.progress * step : 0
        readonly property real dragShift: demo.mode === "move-window" ? demo.progress * step : 0
        anchors.centerIn: parent
        height: Math.min(demo.height, demo.width / 1.6)
        width: height * 1.6
        compact: height < Kirigami.Units.gridUnit * 3

        Repeater {
            model: 4

            MiniWindow {
                required property int index
                readonly property bool grabbed: index === 1
                x: frame.gap + index * frame.step + frame.rowShift + (grabbed ? frame.dragShift : (index === 2 ? -frame.dragShift : 0))
                z: grabbed ? 1 : 0
                y: frame.height * 0.08
                width: frame.columnWidth
                height: frame.height * 0.84
                lift: grabbed && demo.progress > 0.02 ? 1 : 0
                titleHeight: frame.height * 0.12
                compact: frame.compact
            }
        }

        Rectangle {
            readonly property real hold: Math.min(1, demo.progress * 6)
            z: 2
            width: Math.max(6, frame.height * 0.1)
            height: width
            radius: width / 2
            x: frame.gap + frame.step + frame.columnWidth / 2 + frame.rowShift + frame.dragShift - width / 2
            y: frame.height * 0.14 - height / 2
            color: Qt.alpha(Kirigami.Theme.highlightColor, 0.7)
            border.color: Kirigami.Theme.highlightedTextColor
            border.width: 1

            Rectangle {
                anchors.centerIn: parent
                width: parent.width * (1.4 + parent.hold)
                height: width
                radius: width / 2
                color: "transparent"
                border.color: Kirigami.Theme.highlightColor
                border.width: 2
                opacity: 1 - parent.hold * 0.4
            }
        }
    }
}
