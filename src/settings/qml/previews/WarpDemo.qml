import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: demo

    property string choice: "off"
    property real progress: 0

    SequentialAnimation on progress {
        loops: Animation.Infinite
        running: demo.visible
        PauseAnimation { duration: 700 }
        NumberAnimation { from: 0; to: 1; duration: 500; easing.type: Easing.OutCubic }
        PauseAnimation { duration: 1200 }
        NumberAnimation { from: 1; to: 0; duration: 10 }
    }

    readonly property real windowX: width * 0.5
    readonly property real windowY: height * 0.15
    readonly property real windowW: width * 0.42
    readonly property real windowH: height * 0.7
    readonly property point start: Qt.point(width * 0.12, height * 0.55)
    readonly property point end: {
        switch (choice) {
        case "nearest":
            return Qt.point(windowX + 3, start.y);
        case "center-xy":
        case "center-xy-always":
            return Qt.point(windowX + windowW / 2, windowY + windowH / 2);
        default:
            return start;
        }
    }

    Rectangle {
        x: demo.width * 0.04
        y: demo.windowY
        width: demo.windowW
        height: demo.windowH
        radius: 3
        color: Kirigami.Theme.backgroundColor
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.25)
        opacity: 1 - demo.progress * 0.4
    }

    Rectangle {
        x: demo.windowX
        y: demo.windowY
        width: demo.windowW
        height: demo.windowH
        radius: 3
        color: Kirigami.Theme.backgroundColor
        border.color: demo.progress > 0.5 ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.25)
        border.width: demo.progress > 0.5 ? 2 : 1
    }

    Kirigami.Icon {
        width: Kirigami.Units.iconSizes.small
        height: width
        source: "input-mouse-click-left"
        x: demo.start.x + (demo.end.x - demo.start.x) * demo.progress - width / 2
        y: demo.start.y + (demo.end.y - demo.start.y) * demo.progress - height / 2
    }
}
