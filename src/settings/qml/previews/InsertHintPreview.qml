import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: preview

    property var values: ({})
    readonly property var hint: values["insert-hint"] || ({})
    property real t: 0

    SequentialAnimation on t {
        loops: Animation.Infinite
        running: preview.visible
        NumberAnimation { from: 0; to: 1; duration: 1600; easing.type: Easing.InOutCubic }
        PauseAnimation { duration: 700 }
        NumberAnimation { from: 1; to: 0; duration: 900; easing.type: Easing.InOutCubic }
    }

    MockWindow {
        id: left
        x: 0
        y: parent.height * 0.12
        width: parent.width * 0.36
        height: parent.height * 0.8
        caption: "Window"
    }

    MockWindow {
        id: right
        x: parent.width * 0.64
        y: left.y
        width: left.width
        height: left.height
        caption: "Window"
    }

    PaintFrame {
        x: left.width + parent.width * 0.04
        y: left.y
        width: parent.width * 0.2
        height: left.height
        filled: true
        radius: 6
        opacity: preview.hint.enabled ? preview.t * 0.6 : 0
        paint: preview.hint.paint
    }

    MockWindow {
        width: parent.width * 0.22
        height: parent.height * 0.45
        x: parent.width * (0.7 - 0.31 * preview.t)
        y: parent.height * (0.05 + 0.2 * preview.t)
        caption: "Dragging"
        state_: "active"
        opacity: 0.92
        rotation: 3 * (1 - preview.t)
    }
}
