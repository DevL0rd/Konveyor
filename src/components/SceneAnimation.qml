import QtQuick
import org.kde.kirigami as Kirigami
import "Scenes.js" as Scenes
import "ShortcutScenes.js" as ShortcutScenes

Item {
    id: demo

    property string actionId
    property var scene: ShortcutScenes.sceneFor(actionId)
    property bool animated: true
    property bool showKeys: false
    property real clock: 0
    readonly property bool hasScene: scene !== null && scene !== undefined
    readonly property var cursor: hasScene ? (animated ? Scenes.cursor(scene, clock) : Scenes.still(scene)) : null
    readonly property bool compact: height < Kirigami.Units.gridUnit * 3
    property real aspect: 16 / 10
    readonly property var keyboard: {
        const keys = [];
        for (let c = 0; c < 12; ++c) {
            keys.push({ r: 0, c: c, w: 1, name: c === 2 ? "number" : (c === 10 ? "home" : (c === 11 ? "end" : "")) });
        }
        for (let c = 0; c < 10; ++c) {
            keys.push({ r: 1, c: c, w: 1, name: c === 3 ? "letter" : "" });
        }
        keys.push({ r: 1, c: 10, w: 1, name: "up" }, { r: 1, c: 11, w: 1, name: "enter" });
        keys.push({ r: 2, c: 0, w: 1.5, name: "" }, { r: 2, c: 1.5, w: 1.5, name: "meta" }, { r: 2, c: 3, w: 1.5, name: "" }, { r: 2, c: 4.5, w: 4.5, name: "" });
        keys.push({ r: 2, c: 9, w: 1, name: "left" }, { r: 2, c: 10, w: 1, name: "down" }, { r: 2, c: 11, w: 1, name: "right" });
        return keys;
    }

    function columns(list, options) {
        return Scenes.columns(list, options);
    }

    function build(frames, options) {
        return Scenes.scene(frames, options);
    }

    function scroll(widths, mode) {
        return Scenes.scrollFrames(widths, mode);
    }

    onSceneChanged: {
        clock = 0;
        if (loop.running) {
            loop.restart();
        }
    }

    NumberAnimation {
        id: loop
        target: demo
        property: "clock"
        from: 0
        to: demo.hasScene ? Scenes.duration(demo.scene) : 1
        duration: to
        loops: Animation.Infinite
        running: demo.visible && demo.animated && demo.hasScene
    }

    Item {
        id: stage
        width: parent.width
        height: demo.showKeys ? parent.height * 0.7 : parent.height
        readonly property int count: demo.hasScene ? demo.scene.monitors : 1
        readonly property bool stacked: demo.hasScene && demo.scene.stackedMonitors
        readonly property real spacing: count > 1 ? Kirigami.Units.smallSpacing : 0
        readonly property real frameHeight: stacked ? Math.min((height - spacing) / count, width / demo.aspect) : Math.min(height, (width - spacing * (count - 1)) / count / demo.aspect)
        readonly property real frameWidth: frameHeight * demo.aspect

        Repeater {
            model: demo.hasScene ? stage.count : 0

            MiniScreen {
                id: screen
                required property int index
                compact: demo.compact
                width: stage.frameWidth
                height: stage.frameHeight
                x: stage.stacked ? (stage.width - width) / 2 : (stage.width - stage.count * width - stage.spacing * (stage.count - 1)) / 2 + index * (width + stage.spacing)
                y: stage.stacked ? (stage.height - stage.count * height - stage.spacing * (stage.count - 1)) / 2 + index * (height + stage.spacing) : (stage.height - height) / 2

                Repeater {
                    model: demo.scene.windows.length

                    MiniWindow {
                        required property int index
                        readonly property var r: Scenes.rect(demo.scene, index, demo.cursor)
                        compact: demo.compact
                        x: (r.x - screen.index + r.w * (1 - r.s) / 2) * screen.width
                        y: (r.y + r.h * (1 - r.s) / 2) * screen.height
                        z: r.z + r.f
                        width: r.w * r.s * screen.width
                        height: r.h * r.s * screen.height
                        opacity: r.o
                        visible: r.o > 0.01
                        lift: r.f
                        alternate: r.home !== 0 || r.alt > 0.5
                        titleHeight: screen.height * (demo.compact ? 0.14 : 0.1)
                        tabs: r.tabs
                        tab: r.tab
                        tabbed: r.tabbed
                        label: r.label
                    }
                }

                MiniPanel {
                    z: 10
                    anchors.centerIn: parent
                    shown: demo.hasScene && screen.index === 0 ? Scenes.panel(demo.scene, demo.cursor) : 0
                }
            }
        }
    }

    Rectangle {
        id: pad
        visible: demo.showKeys && demo.hasScene
        readonly property real unit: width / 12.4
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: stage.bottom
        anchors.topMargin: Kirigami.Units.smallSpacing
        width: Math.min((stage.stacked ? stage.frameWidth : stage.frameWidth * stage.count) * 0.8, (parent.height - stage.height - Kirigami.Units.smallSpacing) * 12.4 / 3.4)
        height: unit * 3.4
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.25)

        Repeater {
            model: demo.keyboard

            Rectangle {
                required property var modelData
                readonly property bool pressed: modelData.name !== "" && (modelData.name === "meta" || modelData.name === (demo.hasScene ? demo.scene.key : ""))
                readonly property real glow: pressed && demo.cursor ? demo.cursor.glow : 0
                x: pad.unit * 0.2 + modelData.c * pad.unit + pad.unit * 0.08
                y: pad.unit * 0.2 + modelData.r * pad.unit + pad.unit * 0.08
                width: modelData.w * pad.unit - pad.unit * 0.16
                height: pad.unit * 0.84
                radius: Math.max(1, height * 0.18)
                color: pressed ? Qt.alpha(Kirigami.Theme.highlightColor, 0.3 + glow * 0.5) : Qt.alpha(Kirigami.Theme.textColor, 0.12)
                border.color: Kirigami.Theme.highlightColor
                border.width: pressed ? 1 : 0

                Rectangle {
                    visible: parent.glow > 0.01
                    anchors.centerIn: parent
                    width: parent.width + pad.unit * 0.5 * parent.glow
                    height: parent.height + pad.unit * 0.5 * parent.glow
                    radius: parent.radius * 1.5
                    color: "transparent"
                    border.color: Kirigami.Theme.highlightColor
                    border.width: 1.5
                    opacity: parent.glow * 0.8
                }
            }
        }
    }
}
