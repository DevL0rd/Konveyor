import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: demo

    property string device: "touchpad"
    property string gesture: "horizontal"
    property int fingers: 3
    property bool natural: true
    property bool active: true
    property bool showCaption: true
    property bool animated: true
    property real progress: 0
    signal looped

    readonly property bool touchscreen: device === "touchscreen"
    readonly property real handSign: natural ? -1 : 1
    readonly property bool tap: gesture.startsWith("tap-")
    readonly property real tapEffect: tap ? Math.max(0, (progress - 0.3) / 0.7) : 0
    readonly property string caption: {
        const count = fingers + "-finger ";
        switch (gesture) {
        case "horizontal":
            return count + "swipe sideways scrolls the row";
        case "vertical":
            return count + "swipe up or down switches workspace";
        case "pinch":
            return count + "pinch opens the overview";
        case "window-horizontal":
            return count + "swipe sideways merges the window into the next column";
        case "window-vertical":
            return count + "swipe up or down moves the window up or down, then to the next workspace";
        case "long-press":
            return "Hold a title bar, then drag to move the window";
        case "tap-cycle-width":
            return count + "tap cycles the column width";
        case "tap-kontrol-panel":
            return count + "tap opens the Kontrol Panel";
        case "tap-toggle-overview":
            return count + "tap opens the overview";
        }
        return "";
    }

    function columnGeometry(workspaceIndex, index) {
        const h = world.height;
        const lift = world.liftProgress;
        const geometry = { x: world.gap + index * world.step, y: h * 0.08, width: world.columnWidth, height: h * 0.84, highlight: false };
        if (workspaceIndex !== 0) {
            return geometry;
        }
        switch (gesture) {
        case "long-press":
            if (index === 0) {
                geometry.x += lift * world.step;
                geometry.y -= lift * h * 0.03;
                geometry.highlight = progress > 0.35;
            } else if (index === 1) {
                geometry.x -= lift * world.step;
            }
            break;
        case "window-horizontal":
            if (index === 1) {
                geometry.x -= progress * world.step;
                geometry.y += progress * h * 0.43;
                geometry.height -= progress * h * 0.43;
                geometry.highlight = true;
            } else if (index === 0) {
                geometry.height -= progress * h * 0.43;
            } else if (index > 1) {
                geometry.x -= progress * world.step;
            }
            break;
        case "tap-cycle-width":
            if (index === 0) {
                geometry.width += tapEffect * world.step * 0.6;
                geometry.highlight = true;
            } else {
                geometry.x += tapEffect * world.step * 0.6;
            }
            break;
        case "window-vertical":
            if (index === 1) {
                geometry.height = h * 0.41 + world.carryProgress * h * 0.43;
                geometry.y += world.rowProgress * h * 0.43 + world.carryProgress * h * 0.57;
                geometry.highlight = true;
            } else if (index === 2) {
                geometry.x -= world.step;
                geometry.height = h * 0.41 + world.carryProgress * h * 0.43;
                geometry.y += (1 - world.rowProgress) * h * 0.43;
            } else if (index > 2) {
                geometry.x -= world.step;
            }
            break;
        }
        return geometry;
    }

    clip: true
    implicitWidth: Kirigami.Units.gridUnit * 16
    implicitHeight: Kirigami.Units.gridUnit * 10

    SequentialAnimation on progress {
        loops: Animation.Infinite
        running: demo.visible && demo.active && demo.animated
        PauseAnimation { duration: 400 }
        NumberAnimation { from: 0; to: 1; duration: 1400; easing.type: Easing.InOutCubic }
        PauseAnimation { duration: 700 }
        NumberAnimation { from: 1; to: 0; duration: 500; easing.type: Easing.InOutQuad }
        ScriptAction { script: demo.looped() }
    }

    Rectangle {
        id: frame
        readonly property real aspect: 16 / 10
        anchors.horizontalCenter: parent.horizontalCenter
        y: 0
        height: Math.min((demo.height - caption.height) * (demo.touchscreen ? 1 : 0.68), demo.width / aspect)
        width: height * aspect
        radius: Kirigami.Units.cornerRadius * 2
        color: Qt.darker(Kirigami.Theme.backgroundColor, 1.6)
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.35)
        border.width: 2
        clip: true
        opacity: demo.active ? 1 : 0.45

        Item {
            id: world
            readonly property real columnWidth: parent.width * 0.38
            readonly property real gap: parent.width * 0.04
            readonly property real step: columnWidth + gap
            readonly property real liftProgress: Math.max(0, (demo.progress - 0.35) / 0.65)
            readonly property real rowProgress: Math.min(1, demo.progress * 2)
            readonly property real carryProgress: Math.max(0, demo.progress * 2 - 1)
            width: parent.width
            height: parent.height
            transformOrigin: Item.Center
            x: demo.gesture === "horizontal" ? -demo.progress * step : 0
            y: demo.gesture === "vertical" ? -demo.progress * parent.height : (demo.gesture === "window-vertical" ? -carryProgress * parent.height : 0)
            scale: demo.gesture === "pinch" ? 1 - demo.progress * 0.5 : (demo.gesture === "tap-toggle-overview" ? 1 - demo.tapEffect * 0.5 : 1)

            Repeater {
                model: 2

                Item {
                    id: workspace
                    required property int index
                    y: index * world.height
                    width: world.width
                    height: world.height

                    Repeater {
                        model: 4

                        Rectangle {
                            id: column
                            required property int index
                            readonly property var geometry: demo.columnGeometry(workspace.index, index)
                            readonly property bool lifted: geometry.highlight
                            x: geometry.x
                            z: lifted ? 1 : 0
                            y: geometry.y
                            width: geometry.width
                            height: geometry.height
                            radius: 3
                            color: Kirigami.Theme.backgroundColor
                            border.color: lifted ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.25)
                            border.width: lifted ? 2 : 1

                            Rectangle {
                                width: parent.width
                                height: world.height * 0.12
                                radius: 3
                                color: workspace.index === 1 ? Qt.alpha(Kirigami.Theme.positiveTextColor, 0.3) : Qt.alpha(Kirigami.Theme.textColor, 0.1)
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            visible: demo.gesture === "pinch" || demo.gesture === "tap-toggle-overview"
            anchors.fill: parent
            color: "transparent"
            border.color: Kirigami.Theme.highlightColor
            border.width: 2
            radius: 4
            opacity: Math.max(0, (demo.tap ? demo.tapEffect : demo.progress) * 2 - 1)
        }

        Rectangle {
            visible: demo.gesture === "tap-kontrol-panel"
            anchors.centerIn: parent
            width: parent.width * 0.62
            height: parent.height * 0.72
            radius: Kirigami.Units.cornerRadius
            color: Kirigami.Theme.backgroundColor
            border.color: Kirigami.Theme.highlightColor
            border.width: 2
            opacity: demo.tapEffect
            scale: 0.9 + demo.tapEffect * 0.1

            Grid {
                anchors.centerIn: parent
                columns: 4
                spacing: parent.width * 0.05

                Repeater {
                    model: 8

                    Rectangle {
                        width: parent.parent.width * 0.14
                        height: width
                        radius: 3
                        color: Qt.alpha(Kirigami.Theme.highlightColor, 0.35)
                    }
                }
            }
        }
    }

    Rectangle {
        id: pad
        visible: !demo.touchscreen
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: frame.bottom
        anchors.topMargin: Kirigami.Units.smallSpacing
        width: frame.width * 0.5
        height: Math.max(0, demo.height - caption.height - frame.height - Kirigami.Units.smallSpacing * 2)
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.25)
    }

    Item {
        id: hand
        readonly property Item surface: demo.touchscreen ? frame : pad
        readonly property real travel: surface.width * 0.28
        readonly property real spacing: Math.min(surface.width / (demo.fingers + 2), surface.height * 0.3)
        readonly property real dot: Math.max(6, Math.min(spacing * 0.75, Kirigami.Units.gridUnit))
        x: surface.x
        y: surface.y
        width: surface.width
        height: surface.height
        visible: demo.active

        Repeater {
            model: demo.gesture === "long-press" ? 1 : demo.fingers

            Rectangle {
                required property int index
                readonly property real angle: (index / Math.max(1, demo.fingers)) * Math.PI * 2 - Math.PI / 2
                readonly property real rowX: (index - (demo.fingers - 1) / 2) * hand.spacing
                readonly property real baseX: demo.gesture === "pinch" ? Math.cos(angle) * hand.spacing * 1.2 : (demo.gesture === "long-press" ? 0 : rowX)
                readonly property real baseY: demo.gesture === "pinch" ? Math.sin(angle) * hand.spacing * 1.2 : (demo.gesture === "long-press" ? 0 : Math.abs(rowX) * 0.25)
                readonly property real pinchScale: demo.gesture === "pinch" ? 1 - demo.progress * 0.6 : 1
                readonly property real horizontalSign: demo.gesture === "horizontal" ? demo.handSign : (demo.gesture === "window-horizontal" ? -1 : 0)
                readonly property real verticalSign: demo.gesture === "vertical" ? demo.handSign : (demo.gesture === "window-vertical" ? 1 : 0)
                readonly property real moveX: horizontalSign * demo.progress * hand.travel
                readonly property real moveY: verticalSign * demo.progress * hand.height * 0.3
                readonly property real centerX: demo.gesture === "long-press" ? frame.width * 0.04 + world.columnWidth * 0.5 + world.liftProgress * world.step : hand.width / 2 - horizontalSign * hand.travel / 2
                readonly property real centerY: demo.gesture === "long-press" ? frame.height * 0.13 : hand.height / 2 - verticalSign * hand.height * 0.15
                x: centerX + baseX * pinchScale + moveX - width / 2
                y: centerY + baseY * pinchScale + moveY - height / 2
                width: hand.dot
                height: hand.dot
                radius: width / 2
                color: Qt.alpha(Kirigami.Theme.highlightColor, 0.7)
                border.color: Kirigami.Theme.highlightedTextColor
                border.width: 1
                opacity: demo.tap ? Math.max(0, 1 - Math.max(0, demo.progress - 0.25) * 6) : 1

                Rectangle {
                    visible: demo.tap
                    readonly property real press: Math.min(1, demo.progress / 0.25)
                    anchors.centerIn: parent
                    width: hand.dot * (1 + press * 1.2)
                    height: width
                    radius: width / 2
                    color: "transparent"
                    border.color: Kirigami.Theme.highlightColor
                    border.width: 2
                    opacity: 1 - press * 0.7
                }

                Rectangle {
                    visible: demo.gesture === "long-press"
                    readonly property real hold: Math.min(1, demo.progress / 0.35)
                    anchors.centerIn: parent
                    width: hand.dot * (1.4 + hold)
                    height: width
                    radius: width / 2
                    color: "transparent"
                    border.color: Kirigami.Theme.highlightColor
                    border.width: 2
                    opacity: 1 - hold * 0.4
                }
            }
        }
    }

    QQC2.Label {
        id: caption
        visible: demo.showCaption
        anchors.bottom: parent.bottom
        width: parent.width
        height: visible ? implicitHeight : 0
        text: demo.active ? demo.caption : "Left to KDE"
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        opacity: 0.75
        font: Kirigami.Theme.smallFont
    }
}
