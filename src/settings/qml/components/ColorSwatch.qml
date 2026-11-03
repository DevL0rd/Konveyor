import QtQuick

Item {
    id: swatch

    property color color

    Canvas {
        anchors.fill: parent
        visible: swatch.color.a < 1

        onPaint: {
            const context = getContext("2d");
            context.fillStyle = "white";
            context.fillRect(0, 0, width, height);
            context.fillStyle = "black";
            for (let x = 0; x < width; x += 16) {
                for (let y = 0; y < height; y += 16) {
                    context.fillRect(x, y, 8, 8);
                    context.fillRect(x + 8, y + 8, 8, 8);
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: swatch.color
    }
}
