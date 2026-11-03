import QtQuick
import org.kde.kirigami as Kirigami
import "../catalog/EventEffects.js" as EventEffects

Item {
    id: scene

    property var layers: []
    property string reach: "self"
    property real duration: 0
    property bool onScreen: true
    readonly property real pad: Math.round(Kirigami.Units.gridUnit * 0.6)
    readonly property real gap: Math.round(Kirigami.Units.gridUnit * 0.5)
    readonly property real windowWidth: Math.max(1, (width - 2 * pad - 2 * gap) / 3)
    readonly property real windowHeight: Math.max(1, height - 2 * pad)
    readonly property bool crossing: EventEffects.crosses(layers)
    readonly property int subject: crossing ? 2 : 0
    readonly property var play: {
        const rects = [0, 1, 2].map(index => Qt.rect(pad + index * (windowWidth + gap), pad, windowWidth, windowHeight));
        return EventEffects.play(rects, subject, crossing ? 0 : -1, reach, clock.time, duration);
    }

    function measure() {
        const window = windows.itemAt(subject);
        clock.duration = window ? Math.max(0.1, window.frame.eventDuration()) : 1;
    }

    function checkOnScreen() {
        let item = parent;
        while (item && item.contentY === undefined) {
            item = item.parent;
        }
        const top = item ? mapToItem(item, 0, 0).y : 0;
        onScreen = !item || (top + height > 0 && top < item.height);
    }

    clip: true
    onLayersChanged: Qt.callLater(measure)
    onDurationChanged: Qt.callLater(measure)
    onWidthChanged: Qt.callLater(measure)
    onReachChanged: Qt.callLater(measure)
    Component.onCompleted: Qt.callLater(measure)

    Timer {
        interval: 400
        repeat: true
        running: scene.visible
        triggeredOnStart: true
        onTriggered: scene.checkOnScreen()
    }

    EventPlayClock {
        id: clock
        playing: scene.visible && scene.onScreen && scene.layers.length > 0
        still: windows.count > 0 && windows.itemAt(0) !== null && windows.itemAt(0).frame.reduceMotion
    }

    StyleBackdrop {
        anchors.fill: parent
    }

    Repeater {
        id: windows
        model: 3

        Item {
            id: mini

            required property int index
            readonly property alias frame: lit

            x: scene.pad + index * (scene.windowWidth + scene.gap)
            y: scene.pad
            width: scene.windowWidth
            height: scene.windowHeight

            StyleFrame {
                id: lit
                anchors.fill: parent
                layers: EventEffects.reached(mini.index, scene.subject, scene.reach, scene.layers) ? scene.layers : []
                baseWidth: 2
                radius: 3
                play: scene.play ? scene.play[mini.index] : null
            }

            Rectangle {
                anchors.fill: parent
                radius: 3
                color: Kirigami.Theme.backgroundColor
                clip: true

                Rectangle {
                    width: parent.width
                    height: 6
                    color: Qt.alpha(Kirigami.Theme.textColor, mini.index === scene.subject ? 0.32 : 0.1)
                }
            }
        }
    }
}
