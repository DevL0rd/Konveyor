import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/MotionMath.js" as MotionMath
import org.kde.konveyor.settings

Item {
    id: overview

    property bool animationsOn: true
    property real slowdown: 1
    readonly property var motion: MotionMath.motion(SettingsStore.values["animations/horizontal-view-movement"])
    property int focused: 1
    property real offset: 0
    property real from: 0
    property real target: 0
    property real startedAt: 0

    readonly property real columnWidth: Kirigami.Units.gridUnit * 7
    readonly property real gap: Kirigami.Units.gridUnit * 0.6

    function step() {
        focused = (focused + 1) % 6;
        from = offset;
        target = Math.max(0, focused - 1) * (columnWidth + gap);
        startedAt = Date.now();
        if (!animationsOn) {
            offset = target;
            return;
        }
        ticker.start();
    }

    Timer {
        interval: 1500
        repeat: true
        running: overview.visible
        onTriggered: overview.step()
    }

    Timer {
        id: ticker
        interval: 16
        repeat: true
        onTriggered: {
            const elapsed = (Date.now() - overview.startedAt) / Math.max(0.01, overview.slowdown);
            const duration = Math.max(1, overview.motion.durationMs);
            const t = Math.min(elapsed, duration);
            overview.offset = overview.from + (overview.target - overview.from) * overview.motion.valueAt(t);
            if (elapsed >= duration) {
                stop();
            }
        }
    }

    Rectangle {
        id: screen
        anchors.centerIn: parent
        height: parent.height
        width: Math.min(parent.width, height * 16 / 9)
        radius: Kirigami.Units.cornerRadius * 2
        color: Qt.darker(Kirigami.Theme.backgroundColor, 1.4)
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.3)
        border.width: 2
        clip: true

        Repeater {
            model: 6

            Rectangle {
                required property int index
                readonly property bool isFocused: index === overview.focused
                x: overview.gap + index * (overview.columnWidth + overview.gap) - overview.offset
                y: overview.gap
                width: overview.columnWidth
                height: screen.height - overview.gap * 2
                radius: Kirigami.Units.cornerRadius
                color: Kirigami.Theme.backgroundColor
                border.color: isFocused ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.2)
                border.width: isFocused ? 3 : 1

                Rectangle {
                    width: parent.width
                    height: Kirigami.Units.gridUnit
                    radius: parent.radius
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
                }
            }
        }
    }

    Rectangle {
        anchors.horizontalCenter: screen.horizontalCenter
        anchors.bottom: screen.bottom
        anchors.bottomMargin: Kirigami.Units.largeSpacing
        width: caption.implicitWidth + Kirigami.Units.largeSpacing * 2
        height: caption.implicitHeight + Kirigami.Units.smallSpacing * 2
        radius: height / 2
        color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.92)
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.2)

        QQC2.Label {
            id: caption
            anchors.centerIn: parent
            text: overview.animationsOn ? "Scrolling the row, using your settings" : "Animations are off, so the view jumps"
            font: Kirigami.Theme.smallFont
        }
    }
}
