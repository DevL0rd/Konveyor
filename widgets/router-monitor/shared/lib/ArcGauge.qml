/*
 * A 270° arc gauge (speedometer style). Shows `value` against `maxValue`, or an
 * animated sweeping segment while `running`. Center is left empty for the caller
 * to overlay text.
 */
import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: g

    property real value: 0
    property real maxValue: 100
    property string tip: ""                 // hover tooltip; falls back to value/max
    property color color: Kirigami.Theme.highlightColor
    property color trackColor: Qt.alpha(Kirigami.Theme.textColor, 0.12)
    property bool running: false
    property real thickness: Math.max(4, Math.min(width, height) * 0.085)
    property real spin: 0

    NumberAnimation on spin {
        from: 0; to: 1; duration: 1100
        loops: Animation.Infinite
        running: g.running
    }
    onSpinChanged: canvas.requestPaint()
    onValueChanged: canvas.requestPaint()
    onRunningChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var cx = width / 2, cy = height / 2
            var r = Math.min(width, height) / 2 - g.thickness
            if (r <= 0) return
            var start = 135 * Math.PI / 180
            var sweep = 270 * Math.PI / 180
            ctx.lineCap = "round"
            ctx.lineWidth = g.thickness

            // track
            ctx.beginPath()
            ctx.arc(cx, cy, r, start, start + sweep)
            ctx.strokeStyle = g.trackColor
            ctx.stroke()

            if (g.running) {
                // sweeping segment animation
                var seg = 55 * Math.PI / 180
                var a0 = start + (g.spin % 1.0) * sweep
                ctx.beginPath()
                ctx.arc(cx, cy, r, a0, Math.min(a0 + seg, start + sweep))
                ctx.strokeStyle = g.color
                ctx.stroke()
            } else {
                var frac = g.maxValue > 0 ? Math.max(0, Math.min(1, g.value / g.maxValue)) : 0
                if (frac > 0) {
                    ctx.beginPath()
                    ctx.arc(cx, cy, r, start, start + frac * sweep)
                    ctx.strokeStyle = g.color
                    ctx.stroke()
                }
            }
        }
    }

    HoverHandler { id: hover }
    QQC2.ToolTip.visible: hover.hovered && !g.running
    QQC2.ToolTip.text: g.tip !== "" ? g.tip : (Math.round(g.value) + " / " + Math.round(g.maxValue))
    QQC2.ToolTip.delay: 400
}
