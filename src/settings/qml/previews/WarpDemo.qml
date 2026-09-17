import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.konveyor.components

Item {
    id: demo

    property string choice: "off"
    property real progress: 0
    property real shown: 1

    SequentialAnimation {
        loops: Animation.Infinite
        running: demo.visible
        PauseAnimation { duration: 600 }
        NumberAnimation { target: demo; property: "progress"; from: 0; to: 1; duration: 650; easing.type: Easing.InOutCubic }
        PauseAnimation { duration: 1100 }
        NumberAnimation { target: demo; property: "shown"; from: 1; to: 0; duration: 250 }
        PropertyAction { target: demo; property: "progress"; value: 0 }
        NumberAnimation { target: demo; property: "shown"; from: 0; to: 1; duration: 250 }
    }

    MiniScreen {
        id: frame
        readonly property real windowY: height * 0.08
        readonly property real windowW: width * 0.44
        readonly property real windowH: height * 0.84
        readonly property real targetX: width * 0.52
        readonly property point start: Qt.point(width * 0.24, height * 0.55)
        readonly property point end: {
            switch (demo.choice) {
            case "nearest":
                return Qt.point(targetX + width * 0.03, start.y);
            case "center-xy":
            case "center-xy-always":
                return Qt.point(targetX + windowW / 2, windowY + windowH / 2);
            default:
                return start;
            }
        }
        anchors.centerIn: parent
        height: Math.min(demo.height, demo.width / 1.6)
        width: height * 1.6
        compact: height < Kirigami.Units.gridUnit * 3

        MiniWindow {
            x: frame.width * 0.04
            y: frame.windowY
            width: frame.windowW
            height: frame.windowH
            lift: 1 - demo.progress
            titleHeight: frame.height * 0.12
            compact: frame.compact
        }

        MiniWindow {
            x: frame.targetX
            y: frame.windowY
            width: frame.windowW
            height: frame.windowH
            lift: demo.progress
            titleHeight: frame.height * 0.12
            compact: frame.compact
        }

        Kirigami.Icon {
            z: 2
            width: Kirigami.Units.iconSizes.small
            height: width
            source: "input-mouse-click-left"
            opacity: demo.shown
            x: frame.start.x + (frame.end.x - frame.start.x) * demo.progress - width / 2
            y: frame.start.y + (frame.end.y - frame.start.y) * demo.progress - height / 2
        }
    }
}
