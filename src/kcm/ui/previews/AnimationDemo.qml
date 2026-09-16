import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/MotionMath.js" as MotionMath

RowLayout {
    id: demo

    property var params
    property real slowdown: 1
    property bool animationsOn: true
    property real progress: 0
    readonly property real playhead: timer.running ? Math.min(1, elapsed / Math.max(1, motion.durationMs)) : -1
    readonly property var motion: MotionMath.motion(params)
    property real elapsed: 0
    property real startedAt: 0

    function play() {
        elapsed = 0;
        startedAt = Date.now();
        timer.start();
    }

    spacing: Kirigami.Units.largeSpacing

    QQC2.Button {
        icon.name: timer.running ? "media-playback-stop" : "media-playback-start"
        text: timer.running ? "Stop" : "Play"
        enabled: demo.animationsOn
        onClicked: timer.running ? timer.stop() : demo.play()
    }

    Rectangle {
        id: track
        Layout.fillWidth: true
        implicitHeight: Kirigami.Units.gridUnit * 2.4
        radius: Kirigami.Units.cornerRadius * 2
        color: Qt.alpha(Kirigami.Theme.textColor, 0.05)
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.12)
        clip: true

        Rectangle {
            readonly property real travel: track.width - width - Kirigami.Units.smallSpacing * 2
            x: Kirigami.Units.smallSpacing + travel * demo.progress
            anchors.verticalCenter: parent.verticalCenter
            width: Kirigami.Units.gridUnit * 3
            height: parent.height - Kirigami.Units.smallSpacing * 2
            radius: Kirigami.Units.cornerRadius
            color: Qt.alpha(Kirigami.Theme.highlightColor, 0.35)
            border.color: Kirigami.Theme.highlightColor
            border.width: 2
        }
    }

    QQC2.Label {
        text: demo.animationsOn ? Math.round(demo.motion.durationMs * demo.slowdown) + " ms" : "Instant"
        font: Kirigami.Theme.smallFont
        opacity: 0.7
        Layout.preferredWidth: Kirigami.Units.gridUnit * 3.5
    }

    Timer {
        id: timer
        interval: 16
        repeat: true
        onTriggered: {
            demo.elapsed = (Date.now() - demo.startedAt) / Math.max(0.01, demo.slowdown);
            const duration = Math.max(1, demo.motion.durationMs);
            if (demo.elapsed >= duration) {
                demo.progress = 1;
                stop();
                reset.start();
                return;
            }
            demo.progress = demo.motion.valueAt(demo.elapsed);
        }
    }

    Timer {
        id: reset
        interval: 700
        onTriggered: demo.progress = 0
    }
}
