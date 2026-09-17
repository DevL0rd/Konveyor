import QtQuick
import QtQuick.Shapes
import org.kde.kirigami as Kirigami

Shape {
    id: frame

    property var paint
    property real thickness: 4
    property real radius: 6
    property bool filled: false
    readonly property var gradient: paint && paint.gradient ? paint.gradient : null
    readonly property color solid: {
        if (!paint) {
            return "transparent";
        }
        switch (paint.source) {
        case "accent":
            return Kirigami.Theme.highlightColor;
        case "focus":
            return Kirigami.Theme.focusColor;
        case "hover":
            return Kirigami.Theme.hoverColor;
        case "window":
            return Kirigami.Theme.backgroundColor;
        case "window-text":
            return Kirigami.Theme.textColor;
        case "inactive-text":
            return Kirigami.Theme.disabledTextColor;
        default:
            return paint.color || "transparent";
        }
    }
    readonly property real radians: (gradient ? gradient.angle : 180) * Math.PI / 180

    preferredRendererType: Shape.CurveRenderer
    visible: (filled || thickness > 0) && paint !== null && paint !== undefined

    ShapePath {
        strokeWidth: -1
        fillRule: ShapePath.OddEvenFill
        fillColor: frame.solid
        fillGradient: frame.gradient ? linear : null

        PathRectangle {
            x: 0
            y: 0
            width: frame.width
            height: frame.height
            radius: frame.filled ? frame.radius : frame.radius + frame.thickness
        }

        PathRectangle {
            x: frame.filled ? frame.width / 2 : frame.thickness
            y: frame.filled ? frame.height / 2 : frame.thickness
            width: frame.filled ? 0 : frame.width - 2 * frame.thickness
            height: frame.filled ? 0 : frame.height - 2 * frame.thickness
            radius: frame.radius
        }
    }

    LinearGradient {
        id: linear
        x1: frame.width / 2 - Math.sin(frame.radians) * frame.width / 2
        y1: frame.height / 2 + Math.cos(frame.radians) * frame.height / 2
        x2: frame.width / 2 + Math.sin(frame.radians) * frame.width / 2
        y2: frame.height / 2 - Math.cos(frame.radians) * frame.height / 2
        GradientStop { position: 0; color: frame.gradient ? frame.gradient.from : "transparent" }
        GradientStop { position: 1; color: frame.gradient ? frame.gradient.to : "transparent" }
    }
}
