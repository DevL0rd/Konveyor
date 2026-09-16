import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: demo

    property string mode: "scroll-view"
    property bool selected: false
    property real progress: 0

    clip: true

    SequentialAnimation on progress {
        loops: Animation.Infinite
        running: demo.visible
        PauseAnimation { duration: 500 }
        NumberAnimation { from: 0; to: 1; duration: 1300; easing.type: Easing.InOutCubic }
        PauseAnimation { duration: 700 }
        NumberAnimation { from: 1; to: 0; duration: 900; easing.type: Easing.InOutCubic }
    }

    readonly property real columnWidth: width * 0.3
    readonly property real gap: width * 0.04
    readonly property real rowShift: mode === "scroll-view" ? -progress * (columnWidth + gap) : 0
    readonly property real dragShift: mode === "move-window" ? progress * (columnWidth + gap) : 0

    Repeater {
        model: 4

        Rectangle {
            id: column
            required property int index
            readonly property bool grabbed: index === 1
            readonly property real baseX: demo.gap + index * (demo.columnWidth + demo.gap)
            x: baseX + demo.rowShift + (grabbed ? demo.dragShift : (index === 2 ? -demo.dragShift : 0))
            z: grabbed ? 1 : 0
            y: demo.height * 0.08
            width: demo.columnWidth
            height: demo.height * 0.84
            radius: 3
            color: Kirigami.Theme.backgroundColor
            border.color: grabbed ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.25)
            border.width: grabbed ? 2 : 1

            Rectangle {
                width: parent.width
                height: parent.height * 0.18
                radius: 3
                color: column.grabbed ? Qt.alpha(Kirigami.Theme.highlightColor, 0.35) : Qt.alpha(Kirigami.Theme.textColor, 0.1)
            }

            Kirigami.Icon {
                visible: column.grabbed
                width: Math.min(parent.width * 0.4, Kirigami.Units.iconSizes.small)
                height: width
                x: parent.width * 0.45
                y: parent.height * 0.02
                source: "transform-browse"
            }
        }
    }
}
