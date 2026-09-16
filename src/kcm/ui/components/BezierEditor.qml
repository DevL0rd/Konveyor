import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: editor

    property var bezier: [0.25, 0.1, 0.25, 1]
    property var working: bezier.slice()
    signal edited(var bezier)

    readonly property real low: Math.min(0, working[1], working[3]) - 0.1
    readonly property real high: Math.max(1, working[1], working[3]) + 0.1
    readonly property real pad: Kirigami.Units.gridUnit * 0.8

    function toX(value) {
        return pad + value * (width - pad * 2);
    }

    function toY(value) {
        return pad + (height - pad * 2) * (1 - (value - low) / (high - low));
    }

    function fromPoint(px, py) {
        const x = Math.min(1, Math.max(0, (px - pad) / (width - pad * 2)));
        const y = low + (1 - (py - pad) / (height - pad * 2)) * (high - low);
        return [Math.round(x * 100) / 100, Math.round(Math.max(-1, Math.min(2, y)) * 100) / 100];
    }

    onBezierChanged: working = bezier.slice()
    onWorkingChanged: canvas.requestPaint()

    implicitWidth: Kirigami.Units.gridUnit * 11
    implicitHeight: Kirigami.Units.gridUnit * 11

    Rectangle {
        anchors.fill: parent
        radius: Kirigami.Units.cornerRadius * 2
        color: Qt.alpha(Kirigami.Theme.textColor, 0.04)
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.15)
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            const w = editor.working;
            ctx.strokeStyle = Qt.alpha(Kirigami.Theme.textColor, 0.3);
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.moveTo(editor.toX(0), editor.toY(0));
            ctx.lineTo(editor.toX(w[0]), editor.toY(w[1]));
            ctx.moveTo(editor.toX(1), editor.toY(1));
            ctx.lineTo(editor.toX(w[2]), editor.toY(w[3]));
            ctx.stroke();
            ctx.strokeStyle = Kirigami.Theme.highlightColor;
            ctx.lineWidth = 2.5;
            ctx.beginPath();
            ctx.moveTo(editor.toX(0), editor.toY(0));
            ctx.bezierCurveTo(editor.toX(w[0]), editor.toY(w[1]), editor.toX(w[2]), editor.toY(w[3]), editor.toX(1), editor.toY(1));
            ctx.stroke();
        }
    }

    Repeater {
        model: 2

        Rectangle {
            id: handle
            required property int index
            readonly property int base: index * 2

            width: Kirigami.Units.gridUnit * 0.9
            height: width
            radius: width / 2
            x: editor.toX(editor.working[base]) - width / 2
            y: editor.toY(editor.working[base + 1]) - height / 2
            color: drag.containsPress ? Kirigami.Theme.highlightColor : Kirigami.Theme.backgroundColor
            border.color: Kirigami.Theme.highlightColor
            border.width: 2

            Accessible.role: Accessible.Slider
            Accessible.name: "Control point " + (index + 1)

            MouseArea {
                id: drag
                anchors.fill: parent
                anchors.margins: -Kirigami.Units.smallSpacing
                cursorShape: Qt.SizeAllCursor
                preventStealing: true
                onPositionChanged: mouse => {
                    const point = mapToItem(editor, mouse.x, mouse.y);
                    const next = editor.working.slice();
                    const values = editor.fromPoint(point.x, point.y);
                    next[handle.base] = values[0];
                    next[handle.base + 1] = values[1];
                    editor.working = next;
                }
                onReleased: editor.edited(editor.working)
            }
        }
    }

    QQC2.Label {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: Kirigami.Units.smallSpacing
        text: editor.working.map(value => value.toFixed(2)).join("  ")
        font.family: "monospace"
        font.pointSize: Kirigami.Theme.smallFont.pointSize
        opacity: 0.6
    }
}
