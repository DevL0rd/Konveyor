import QtQuick
import QtQuick.Shapes

Shape {
    id: backdrop

    property bool bands: false
    property real topRadius: 0

    preferredRendererType: Shape.CurveRenderer

    ShapePath {
        strokeWidth: -1
        fillGradient: backdrop.bands ? split : diagonal

        PathRectangle {
            x: 0
            y: 0
            width: backdrop.width
            height: backdrop.height
            topLeftRadius: backdrop.topRadius
            topRightRadius: backdrop.topRadius
        }
    }

    LinearGradient {
        id: diagonal
        x1: 0
        y1: 0
        x2: backdrop.width
        y2: backdrop.height
        GradientStop { position: 0; color: "#14161a" }
        GradientStop { position: 0.62; color: "#2f3238" }
        GradientStop { position: 1; color: "#6a6d72" }
    }

    LinearGradient {
        id: split
        x1: 0
        y1: 0
        x2: backdrop.width
        y2: 0
        GradientStop { position: 0; color: "#14161a" }
        GradientStop { position: 0.499; color: "#14161a" }
        GradientStop { position: 0.501; color: "#c9ccd1" }
        GradientStop { position: 1; color: "#c9ccd1" }
    }
}
