import QtQuick
import org.kde.kirigami as Kirigami
import "../catalog/MotionMath.js" as MotionMath

Canvas {
    id: graph

    property var params
    property bool compact: false
    property real playhead: -1
    property color lineColor: Kirigami.Theme.highlightColor
    readonly property var motion: MotionMath.motion(params)

    implicitWidth: Kirigami.Units.gridUnit * 14
    implicitHeight: Kirigami.Units.gridUnit * 7
    antialiasing: true

    onParamsChanged: requestPaint()
    onPlayheadChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onLineColorChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        const pad = compact ? 3 : 10;
        const w = width - pad * 2;
        const h = height - pad * 2;
        const steps = Math.max(24, Math.round(w / 2));
        const duration = Math.max(1, motion.durationMs);
        const values = [];
        let low = 0;
        let high = 1;
        for (let i = 0; i <= steps; ++i) {
            const value = motion.valueAt(duration * i / steps);
            values.push(value);
            low = Math.min(low, value);
            high = Math.max(high, value);
        }
        const toY = value => pad + h - (value - low) / (high - low) * h;
        if (!compact) {
            ctx.strokeStyle = Qt.alpha(Kirigami.Theme.textColor, 0.15);
            ctx.lineWidth = 1;
            ctx.setLineDash([4, 4]);
            for (const level of [0, 1]) {
                ctx.beginPath();
                ctx.moveTo(pad, toY(level));
                ctx.lineTo(pad + w, toY(level));
                ctx.stroke();
            }
            ctx.setLineDash([]);
        }
        ctx.beginPath();
        values.forEach((value, i) => {
            const x = pad + w * i / steps;
            if (i === 0) {
                ctx.moveTo(x, toY(value));
            } else {
                ctx.lineTo(x, toY(value));
            }
        });
        ctx.strokeStyle = lineColor;
        ctx.lineWidth = compact ? 2 : 2.5;
        ctx.stroke();
        if (!compact) {
            ctx.lineTo(pad + w, toY(low));
            ctx.lineTo(pad, toY(low));
            ctx.closePath();
            ctx.fillStyle = Qt.alpha(lineColor, 0.12);
            ctx.fill();
        }
        if (playhead >= 0 && playhead <= 1) {
            const x = pad + w * playhead;
            const y = toY(motion.valueAt(duration * playhead));
            ctx.fillStyle = lineColor;
            ctx.beginPath();
            ctx.arc(x, y, 4, 0, Math.PI * 2);
            ctx.fill();
        }
    }
}
